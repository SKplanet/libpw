/*
 * The MIT License (MIT)
 * Copyright (c) 2015 SK PLANET. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*!
 * \file pw_instance_if.h
 * \brief Instance(framework) interface.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_instance_if.h"
#include "./pw_string.h"
#include "./pw_iopoller_epoll.h"
#include "./pw_ssl.h"
#include "./pw_crypto.h"
#include "./pw_digest.h"
#include "./pw_compress.h"

#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

namespace pw
{

typedef struct _child_info_type
{
	size_t	index;
	pid_t	pid;
	int		exit_status;
	void*	param;

	inline _child_info_type(size_t _index, pid_t _pid, int _exit_status, void* _param) : index(_index), pid(_pid), exit_status(_exit_status), param(_param) {}
} _child_info_type;

typedef std::list<_child_info_type>	_child_info_cont;
typedef _child_info_cont::iterator	_child_info_itr;

static char s_header[1024] {0x00};

static
void
_sigHUP(int sig)
{
	if (InstanceInterface::s_inst) InstanceInterface::s_inst->setFlagReload();
}

static
void
_sigCHLD(int)
{
	if (InstanceInterface::s_inst) InstanceInterface::s_inst->setFlagCheckChild();
}

static
void
_sigUSR1(int)
{
	if (InstanceInterface::s_inst) InstanceInterface::s_inst->setFlagRun(false);
}

static
void
_sigUSR2(int)
{
	if (InstanceInterface::s_inst) InstanceInterface::s_inst->setFlagRun(false);
}

static
void
_sigINT(int)
{
	if (InstanceInterface::s_inst) InstanceInterface::s_inst->setFlagRun(false);
}

InstanceInterface* InstanceInterface::s_inst(nullptr);

InstanceInterface::_WakeUp::_WakeUp(InstanceInterface& inst) : m_inst(inst)
{
	if ( -1 == (m_fd = ::socket(PF_UNIX, SOCK_STREAM, 0)) )
	{
		PWLOGLIB("%s failed to initialize wakeup client", s_header);
	}
}

InstanceInterface::_WakeUp::~_WakeUp()
{
	if ( -1 not_eq m_fd )
	{
		IoPoller* poller( m_inst.getPoller() );
		if ( poller ) poller->remove(m_fd);
		::close(m_fd);
		m_fd = -1;
	}
}

void
InstanceInterface::_WakeUp::reopen(void)
{
	if ( -1 not_eq m_fd )
	{
		IoPoller* poller( m_inst.getPoller() );
		if ( poller ) poller->remove(m_fd);
		::close(m_fd);
		m_fd = -1;
	}
	if ( -1 == ( m_fd = ::socket(PF_UNIX, SOCK_STREAM, 0)) )
	{
		PWLOGLIB("%s failed to initialize wakeup client", s_header);
	}
}

bool
InstanceInterface::_WakeUp::setEventOut(void)
{
	IoPoller* poller(m_inst.getPoller());
	if ( nullptr == poller || -1 == m_fd )
	{
		PWLOGLIB("%s failed to wake up poller", s_header);
		return false;
	}
	return poller->add(m_fd, this, POLLOUT);
}

void
InstanceInterface::_WakeUp::eventIo(int fd, int e, bool& del_this)
{
	//PWTRACE("eventIo!!!!! wakeup!!!!!");
	del_this = true;
}

InstanceInterface::InstanceInterface(const char* tag) : m_exit_code(EXIT_SUCCESS), m_wakeup(*this)
{
	struct _start& start(const_cast<struct _start&>(m_start));
	start.parent = start.child = time(nullptr);
	m_config.path = "./config.ini";
	m_lsnrs["svc"].port = "30010";
	m_lsnrs["svcssl"].port = "30011";
	m_lsnrs["http"].port = "30010";
	m_lsnrs["https"].port = "30011";
	m_lsnrs["admin"].port = "40010";
	m_lsnrs["adminssl"].port = "40011";
	m_timeout.job = 1000000LL;
	m_timeout.ping = 1000000LL * 90;
	m_app.tag = tag;
	//PWStr::format(m_app.name, "%s:%d", tag, int(getpid()));
	m_poller.type = "auto";
	m_poller.poller = nullptr;
	m_poller.timeout = 500;
	m_flag.run = true;
	m_flag.reload = false;
	m_flag.stage = false;
	m_args.count = 0;
	m_args.value = nullptr;
	m_child.type = ProcessType::SINGLE;
	m_child.count = 0;
	m_child.dead_count = 0;
	m_child.index = size_t(-1);
	m_child.cont = nullptr;
	s_inst = this;
};

InstanceInterface::~InstanceInterface()
{
}

void InstanceInterface::logError ( const char* file, size_t line, const char* fmt, ... )
{
	va_list lst;
	va_start(lst, fmt);
	Log::s_logLibraryV(file, line, fmt, lst);
	va_end(lst);
}

void InstanceInterface::logErrorV ( const char* file, size_t line, const char* fmt, va_list lst )
{
	Log::s_logLibraryV(file, line, fmt, lst);
}

InstanceInterface::lsnr_type*
InstanceInterface::getListenerInfo(const ListenerInterface* lsnr, const lsnr_type* e)
{
	auto ib(std::find_if_not(m_lsnrs.begin(), m_lsnrs.end(),
	[e] (const lsnr_cont::value_type& v) { return &(v.second) == e; }
							));
	return (ib not_eq m_lsnrs.end() ? &(ib->second) : nullptr);
}

const InstanceInterface::lsnr_type*
InstanceInterface::getListenerInfo(const ListenerInterface* lsnr, const lsnr_type* e) const
{
	auto ib(std::find_if_not(m_lsnrs.begin(), m_lsnrs.end(),
	[e] (const lsnr_cont::value_type& v) { return &(v.second) == e; }
							));
	return (ib not_eq m_lsnrs.end() ? &(ib->second) : nullptr);
}

void
InstanceInterface::setListenerEmpty(const ListenerInterface* lsnr)
{
	for(auto& info : m_lsnrs )
	{
		if ( lsnr == info.second.lsnr )
		{
			info.second.lsnr = nullptr;
		}
	}
}

void
InstanceInterface::setStartBuild(const char* build_date, const char* build_time)
{
	if ( m_start.build not_eq 0 )
	{
		PWABORT("start build time is already set");
	}
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s %s", __DATE__, __TIME__);
	struct tm tm;
	memset(&tm, 0x00, sizeof(tm));
	if ( nullptr == strptime(buf, "%b %d %Y %H:%M:%S", &tm) )
	{
		PWABORT("invalid date or time format: build_date:%s build_time:%s", build_date, build_time);
	}
	const_cast<time_t&>(m_start.build) = ::mktime(&tm);
}

bool
InstanceInterface::s_initLibraries(void)
{
	if ( not ssl::initialize() ) return false;

#if (HAVE_EVP_CIPHER_CTX > 0)
	if ( not crypto::initialize() ) return false;
#endif

#if (HAVE_EVP_MD_CTX > 0 )
	if ( not digest::initialize() ) return false;
#endif

	Compress::s_initialize();
	return true;
}

bool
InstanceInterface::s_initSignals(void)
{
	::signal(SIGPIPE, SIG_IGN);	// ignore
	::signal(SIGALRM, SIG_IGN);	// ignore
	::signal(SIGHUP, _sigHUP);	// reload config
	::signal(SIGCHLD, _sigCHLD);	// check child
	::signal(SIGUSR1, _sigUSR1);	// exit
	::signal(SIGUSR2, _sigUSR2);	// exit
	::signal(SIGINT, _sigINT);	// exit
	return true;
}

int
InstanceInterface::start(int argc, char* argv[])
{
	snprintf( s_header, sizeof(s_header), "[%d:%d]", static_cast<int>(getpid()), (isChild() ? static_cast<int>(m_child.index) : -1));
	PWTRACE("%s STARTING", s_header);
	m_args.count = argc;
	m_args.value = argv;
	PWTRACE("%s s_initLibraries", s_header);
	if ( not s_initLibraries() )
	{
		PWLOGLIB("%s failed to initialize libraries", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	if ( not isChild() )
	{
		PWTRACE("%s s_initSignals", s_header);
		if ( not s_initSignals() )
		{
			PWLOGLIB("%s failed to initialize signals", s_header);
			m_exit_code = EXIT_FAILURE;
			return m_exit_code;
		}
		PWTRACE("%s m_sysinfo.getAll()", s_header);
		if ( not m_sysinfo.getAll() )
		{
			PWLOGLIB("%s failed to get system information", s_header);
			m_exit_code = EXIT_FAILURE;
			return m_exit_code;
		}
	}
	if ( argc >= 2 )
	{
		m_config.path = argv[1];
	}
	PWTRACE("%s loadConfig", s_header);
	if ( not loadConfig(m_config.path.c_str(), true, isChild()) )
	{
		PWLOGLIB("%s failed to load config: path:%s", s_header, m_config.path.c_str());
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	PWTRACE("%s eventInitLog", s_header);
	if ( not eventInitLog() )
	{
		PWLOGLIB("%s failed to initialize log", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	Log::s_setLibrary(&m_log.err);
	PWTRACE("%s createPoller", s_header);
	if ( nullptr == (m_poller.poller = IoPoller::s_create(m_poller.type.c_str()) ) )
	{
		PWLOGLIB("%s failed to initialize poller: type:%s", s_header, m_poller.type.c_str());
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	else
	{
		PWLOGLIB("%s success to initialize poller: type:%s", s_header, m_poller.poller->getType());
	}
	PWTRACE("%s add wakeup", s_header);
	if ( not m_poller.poller->add(m_wakeup.m_fd, &m_wakeup, POLLOUT) )
	{
		PWLOGLIB("%s failed to add wakeup instance", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	PWTRACE("%s eventInitChannel", s_header);
	if ( not eventInitChannel() )
	{
		PWLOGLIB("%s failed to initialize channels", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	PWTRACE("%s eventInitListener", s_header);
	if ( not eventInitListener() )
	{
		PWLOGLIB("%s failed to initialize listener", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	if ( (m_child.count > 0)
			and (ProcessType::MULTI == m_child.type)
			and (size_t(-1) == m_child.index) )
	{
		PWTRACE("%s eventInitChild", s_header);
		if ( not eventInitChild() )
		{
			PWLOGLIB("%s failed to initialize child: count:%zu", s_header, m_child.count);
			m_exit_code = EXIT_FAILURE;
			return m_exit_code;
		}
		if ( isChild() )
		{
			int res(start(argc, argv));
			::exit(res);
		}
	}
	PWTRACE("%s eventInitTimer", s_header);
	if ( not eventInitTimer() )
	{
		PWLOGLIB("%s failed to initialize timer", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	PWTRACE("%s eventInitExtras", s_header);
	if ( not eventInitExtras() )
	{
		PWLOGLIB("%s failed to initialize extras", s_header);
		m_exit_code = EXIT_FAILURE;
		return m_exit_code;
	}
	bool& run(m_flag.run);
	bool& check_child(m_flag.check_child);
	bool& reload(m_flag.reload);
	IoPoller& poller(*m_poller.poller);
	PWTRACE("%s start loop", s_header);
	while ( run )
	{
		if ( check_child )
		{
			PWTRACE("check dead child...");
			check_child = false;
			_checkChild();
		}
		if ( reload )
		{
			PWTRACE("%s reload config...", s_header);
			reload = false;
			loadConfig(nullptr, true, true);
		}
		if ( poller.dispatch(m_poller.timeout) < 0 )
		{
			PWLOGLIB("%s poller error: errno:%d %s", s_header, errno, strerror(errno));
			// do nothing...
		}
		m_job.man.checkTimeout(m_timeout.job);
		Timer::s_getInstance().check();
		eventEndTurn();
	}
	eventExit();
	return m_exit_code;
}

void
InstanceInterface::_checkChild(void)
{
	_child_info_cont cont;
	pid_t pid;
	int status;
	child_type* ci(nullptr);
	do
	{
		if ( 0 > ( pid = waitpid(-1, &status, WNOHANG)) )
		{
			if ( (EINTR == errno) or (EAGAIN == errno) ) continue;
			break;
		}
		else if ( 0 == pid ) break;
		if ( nullptr != ( ci = getChildByPID(pid) ) )
		{
			cont.push_back(_child_info_type(ci->index, pid, status, ci->param));
			ci->pid = -1;
			ci->param = nullptr;
			_closePair(ci->fd);
			break;
		}
		else
		{
			cont.push_back(_child_info_type(size_t(-1), pid, status, nullptr));
		}
	}
	while (true);
	_child_info_itr ib(cont.begin());
	_child_info_itr ie(cont.end());
	while ( ib != ie )
	{
		++m_child.dead_count;
		eventExitChild(ib->index, ib->pid, ib->exit_status, ib->param);
		++ib;
	}
}

void
InstanceInterface::_closePair(int fd[2])
{
	for ( size_t i(0); i<2; i++ )
	{
		if ( fd[i] != -1 )
		{
			if ( m_poller.poller ) m_poller.poller->remove(fd[i]);
			::close(fd[i]);
		}
	}
}

bool
InstanceInterface::loadConfig(const char* path, bool isDefault, bool isReload)
{
	if ( nullptr != path ) m_config.path = path;
	if ( not m_config.conf.read(m_config.path.c_str()) )
	{
		PWLOGLIB("failed to read config file: path:%s isDefault:%d isReload:%d", m_config.path.c_str(), int(isDefault), int(isReload));
		return false;
	}
	if ( isDefault )
	{
		if ( not loadDefaultConfig(isReload) )
		{
			PWLOGLIB("failed to read config file: path:%s isDefault:%d isReload:%d", m_config.path.c_str(), int(isDefault), int(isReload));
			return false;
		}
	}
	return eventConfig(isDefault, isReload);
}

bool InstanceInterface::loadPort ( const std::string& tag, Ini::sec_citr sec )
{
	auto& port(getListenerInfo(tag).port);
	Ini& conf(m_config.conf);

	conf.getString2(port, PWStr::formatS("%s.port", cstr(tag)), sec);
	return not port.empty();
}

bool
InstanceInterface::loadPort ( const std::string& tag, const std::string& sec )
{
	Ini& conf(m_config.conf);
	auto isec(conf.find(sec));
	if ( isec == conf.end() ) return false;

	return loadPort(tag, isec);
}

bool
InstanceInterface::loadDefaultConfig(bool isReload)
{
	Ini& conf(m_config.conf);
	Ini::sec_citr sec(conf.find("main"));
	if ( conf.cend() == sec )
	{
		PWLOGLIB("no main section: path:%s isReload:%d", m_config.path.c_str(), int(isReload));
		return false;
	}
	if ( not isReload )
	{
		if ( (m_app.name = conf.getString2(m_app.name, "app.name", sec, m_app.name)).empty() )
		{
			PWLOGLIB("no app.name item at main section");
			return false;
		}
		PWTRACE("app.name: %s", m_app.name.c_str());

		loadPort("svc", sec);
		loadPort("svcssl", sec);
		loadPort("http", sec);
		loadPort("https", sec);
		loadPort("admin", sec);
		loadPort("adminssl", sec);

		conf.getString2(m_poller.type, "poller.type", sec, m_poller.type);
		PWTRACE("poller.type: %s", m_poller.type.c_str());
		if ( true == (m_flag.stage = conf.getBoolean("flag.stage", sec, m_flag.stage)) )
		{
			PWLOGLIB("STAGE MODE!");
		}
		do
		{
			std::string tmp("single");
			tmp = conf.getString2(tmp, "child.type", sec, tmp);
			m_child.type = ( 0 == strcasecmp(tmp.c_str(), "multi") ) ? ProcessType::MULTI : ProcessType::SINGLE;
		}
		while(false);
		m_child.count = conf.getInteger("child.count", sec, m_child.count);
		if ( m_child.count == 0 ) m_child.type = ProcessType::SINGLE;
		if ( m_child.type == ProcessType::SINGLE ) m_child.count = 0;
		PWTRACE("child.type: %s", m_child.type == ProcessType::MULTI ? "multi" : "single");
		PWTRACE("child.count: %zu", m_child.count);
		if ( not isSingle() )
		{
			if ( not _initChildInfo() )
			{
				PWLOGLIB("failed to initialize child process info");
				return false;
			}
		}// if (not isSingle())
	}
	//--------------------------------------------------------------------------
	// isReload에도 읽을 설정
	m_timeout.job = conf.getInteger("timeout.job", sec, m_timeout.job);
	PWTRACE("timeout.job: %jd", intmax_t(m_timeout.job));
	m_timeout.ping = conf.getInteger("timeout.ping", sec, m_timeout.job);
	PWTRACE("timeout.ping: %jd", intmax_t(m_timeout.ping));
	reloadLog(m_log.cmd, m_config.conf, "cmd", "log.cmd", "main");
	reloadLog(m_log.err, m_config.conf, "err", "log.err", "main");
	if ( 0 > (m_poller.timeout = conf.getInteger("poller.timeout", sec, m_poller.timeout)) )
	{
		PWLOGLIB("invalid poller.timeout: %jd", intmax_t(m_poller.timeout));
		return false;
	}
	PWTRACE("poller.timeout: %jd", intmax_t(m_poller.timeout));
	bool use_trace(Log::s_getTrace());
	Log::s_setTrace(conf.getBoolean("log.trace", sec, use_trace));
	return true;
}

bool
InstanceInterface::_initChildInfo(void)
{
	if ( nullptr == (m_child.cont = new child_type[m_child.count]) )
	{
		PWLOGLIB("not enough memory");
		return false;
	}
	for ( size_t i(0); i < m_child.count; i++ )
	{
		child_type& ci( m_child.cont[i] );
		ci.index = i;
		ci.pid = -1;
		ci.param = nullptr;
		ci.fd[0] = ci.fd[1] = -1;
	}
	return true;
}

char*
InstanceInterface::makeLogPrefix(char* obuf, size_t obuflen, const char* typetag) const
{
	if ( isSingle() )
	{
		snprintf(obuf, obuflen, "%s_%s.", m_app.tag.c_str(), typetag);
	}
	else if ( isChild() )
	{
		snprintf(obuf, obuflen, "child%zu_%s.", getChildIndex()+1, typetag);
	}
	else
	{
		snprintf(obuf, obuflen, "parent_%s.", typetag);
	}
	return obuf;
}

std::string&
InstanceInterface::makeLogPrefix(std::string& obuf, const std::string& typetag) const
{
	if ( isSingle() )
	{
		PWStr::format(obuf, "%s_%s.", m_app.tag.c_str(), typetag.c_str());
	}
	else if ( isChild() )
	{
		PWStr::format(obuf, "child%zu_%s.", getChildIndex()+1, typetag.c_str());
	}
	else
	{
		PWStr::format(obuf, "parent_%s.", typetag.c_str());
	}
	return obuf;
}

bool
InstanceInterface::reloadLog(Log& log, const Ini& ini, const std::string& typetag, const std::string& ini_item, const std::string& ini_section)
{
	Log::unique_lock_type locker(log.m_lock);
	char item[1024];
	std::string path, rotate_str, prefix;
	Log::Rotate rtype(log.m_rotate_type);
	snprintf(item, sizeof(item), "%s.path", ini_item.c_str());
	ini.getString2(path, item, ini_section, log.m_path);
	snprintf(item, sizeof(item), "%s.rotate", ini_item.c_str());
	ini.getString2(rotate_str, item, ini_section);
	if ( not strcasecmp(rotate_str.c_str(), "DAILY") ) rtype = Log::Rotate::DAILY;
	else if ( not strcasecmp(rotate_str.c_str(), "HOURLY") ) rtype = Log::Rotate::HOURLY;
	makeLogPrefix(prefix, typetag);

	return log._reopen(path.c_str(), prefix.c_str(), rtype);
}

bool
InstanceInterface::initializeLog(Log& log, const char* typetag, const char* path, Log::Rotate rtype)
{
	{
		Log::unique_lock_type locker(log.m_lock);
		char tag[1024];
		if ( isSingle() )
		{
			snprintf(tag, sizeof(tag), "%s_%s.", m_app.tag.c_str(), typetag);
		}
		else if ( isChild() )
		{
			snprintf(tag, sizeof(tag), "child%zu_%s.", getChildIndex()+1, typetag);
		}
		else
		{
			snprintf(tag, sizeof(tag), "parent_%s.", typetag);
		}
		if ( not log._open(path, tag, rtype) )
		{
			PWLOGLIB("failed to open log: typetag:%s path:%s", typetag, path);
			return false;
		}
		log._reopen();
	}// empty scope
	log.log("**** %s log start: pid:%d ****", typetag, int(getpid()));
	return true;
}

bool
InstanceInterface::fork(size_t index, void* param)
{
	if ( isChild() )
	{
		PWLOGLIB("%s not allowed to fork by child process: pid:%d", s_header, int(getpid()));
		return false;
	}
	if ( index >= m_child.count )
	{
		PWLOGLIB("%s invalid index: index:%zu count:%zu", s_header, index, m_child.count);
		return false;
	}
	child_type& child(m_child.cont[index]);
	_closePair(child.fd);
	if ( -1 == ::socketpair(PF_LOCAL, SOCK_STREAM, 0, child.fd) )
	{
		PWLOGLIB("%s failed to initialize pair socket(%d): %s", s_header, errno, strerror(errno));
		return false;
	}
	pid_t pid(::fork());
	if ( pid == -1 )
	{
		PWLOGLIB("%s failed to fork(%d): %s", s_header, errno, strerror(errno));
		return false;
	}
	if ( pid == 0 )
	{
		m_child.index = index;
		child.pid = pid;
		child.param = param;
		// 자신의 객체를 제외한 다른 객체는 삭제한다.
		for ( size_t i(0); i<m_child.count; i++ )
		{
			if ( i == index ) continue;
			child_type& ci(m_child.cont[i]);
			ci.pid = -1;
			ci.param = nullptr;
			_closePair(ci.fd);
		}
		struct _start& start(const_cast<struct _start&>(m_start));
		start.child = time(nullptr);
		//::signal(SIGCHLD, SIG_IGN);
		::sleep(1);
		eventForkChild(index, param);
		eventExit();
		exit(m_exit_code);
	}
	else
	{
		child.pid = pid;
		child.param = param;
	}
	PWLOGLIB("success to fork: pid:%d index:%zu", int(pid), index);
	return true;
}

bool
InstanceInterface::wakeUp(void)
{
	return m_wakeup.setEventOut();
}

void
InstanceInterface::cleanUpForChild(size_t index, void* param)
{
#if defined(HAVE_EPOLL)
	IoPoller_Epoll* poller(dynamic_cast<IoPoller_Epoll*>(m_poller.poller));
	if ( poller ) poller->destroy();
#endif
	m_wakeup.reopen();
	eventForkCleanUpChannel(index, param);
	eventForkCleanUpListener(index, param);
	eventForkCleanUpExtras(index, param);
	eventForkCleanUpPoller(index, param);
	eventForkCleanUpTimer(index, param);
}

//==============================================================================
// Events...

bool
InstanceInterface::eventInitChild(void)
{
	PWTRACE("child count: %zu", m_child.count);
	for ( size_t i(0); i < m_child.count; i++ )
	{
		if ( not fork(i, nullptr) )
		{
			return false;
		}
	}
	return true;
}

void
InstanceInterface::eventExit(void)
{
	PWLOGLIB("**** %s end: pid:%d ****", m_app.name.c_str(), int(getpid()));
	if ( not isChild() )
	{
		// 혼자 죽지 말자. ;-)
		signalToChildAll_Interrupt();
	}
}

void
InstanceInterface::eventExitChild(size_t index, pid_t pid, int exit_status, void* param)
{
	PWLOGLIB("child is dead: index:%zu pid:%d exit_status:%d param:%p", index, int(pid), exit_status, param);
}

void
InstanceInterface::eventForkChild(size_t index, void* param)
{
	PWLOGLIB("child is forked: index:%zu ppid:%d pid:%d param:%p", index, int(::getppid()), int(::getpid()), param);
	cleanUpForChild(index, param);
	start(m_args.count, m_args.value);
}

void
InstanceInterface::eventForkCleanUpListener(size_t index, void* param)
{
	PWTRACE("%s: index:%zu ppid:%d pid:%d param:%p", __func__, index, int(::getppid()), int(::getpid()), param);
	for(auto& lsnr : m_lsnrs )
	{
		lsnr_type& info(lsnr.second);
		if ( info.lsnr )
		{
			ListenerInterface* lsnr(info.lsnr);
			lsnr->close();
			delete lsnr;
			setListenerEmpty(lsnr);
		}
	}
}

void
InstanceInterface::eventForkCleanUpTimer(size_t index, void* param)
{
	PWTRACE("%s: index:%zu ppid:%d pid:%d param:%p", __func__, index, int(::getppid()), int(::getpid()), param);
	Timer::s_getInstance().clear();
}

void
InstanceInterface::eventForkCleanUpPoller(size_t index, void* param)
{
	PWTRACE("%s: index:%zu ppid:%d pid:%d param:%p", __func__, index, int(::getppid()), int(::getpid()), param);
	if ( m_poller.poller )
	{
		IoPoller::s_release(m_poller.poller);
		m_poller.poller = nullptr;
	}
}

bool
InstanceInterface::signalToChild(int signal, size_t idx) const
{
	if ( isChild() ) return false;
	auto child(getChildByIndex(idx));
	if ( nullptr == child ) return false;
	return 0 == ::kill(child->pid, signal);
}

bool
InstanceInterface::signalToChildAll(int signal) const
{
	bool bRes(true);
	for ( size_t i(0); i<getChildCount(); i++ )
	{
		bRes &= signalToChild(signal, i);
	}
	return bRes;
}

bool
InstanceInterface::signalToChild_Interrupt(size_t idx) const
{
	return signalToChild(SIGINT, idx);
}

bool
InstanceInterface::signalToChildAll_Interrupt(void) const
{
	return signalToChildAll(SIGINT);
}

bool
InstanceInterface::signalToChild_User1(size_t idx) const
{
	return signalToChild(SIGUSR1, idx);
}

bool
InstanceInterface::signalToChildAll_User1(void) const
{
	return signalToChildAll(SIGUSR1);
}

bool
InstanceInterface::signalToChild_User2(size_t idx) const
{
	return signalToChild(SIGUSR2, idx);
}

bool
InstanceInterface::signalToChildAll_User2(void) const
{
	return signalToChildAll(SIGUSR2);
}

bool
InstanceInterface::signalToChild_Kill(size_t idx) const
{
	return signalToChild(SIGKILL, idx);
}

bool
InstanceInterface::signalToChildAll_Kill(void) const
{
	return signalToChildAll(SIGKILL);
}


};//namespace pw

