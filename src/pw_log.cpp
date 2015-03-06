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
 * \file pw_log.cpp
 * \brief Simple log system.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>

namespace pw {

#define _DEF_BUFLEN (1024*10)
#define _DEF_BUF(x) char x[_DEF_BUFLEN] = {0x00}

#define _PWLOG_OPENMODE (O_WRONLY|O_APPEND|O_CREAT|O_LARGEFILE|O_SYNC)
#define _PWLOG_OPENFLAG (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

#define GLOBAL_LOCK() unique_lock_type locker(s_lock)
#define LOCAL_LOCK() unique_lock_type locker(m_lock)
#define BOTH_LOCK() std::lock(s_lock, m_lock)

#define _STR_TRACE "TRACE"
#define _STR_DEBUG "DEBUG"
#define _STR_INFO "INFO"
#define _STR_WARN "WARN"
#define _STR_ERROR "ERROR"
#define _STR_FATAL "FATAL"

#if 1
#	define LINK(x,y)	::link(x,y)
#else
#	define LINK(x,y)	::symlink(x,y)
#endif

static char s_lf[2] = { '\r', '\n' };

inline
const char*
__getFilename(const char* path)
{
	const char* r(strrchr(path, '/'));
	if ( nullptr == r ) return path;
	return (r+1);
}

bool Log::s_use_trace(true);
Log* Log::s_log_library;
std::set<intptr_t> Log::s_logs;
std::mutex Log::s_lock;
static const std::map<std::string, Log::Level, str_ci_less<std::string>> s_str2lv = {
	{_STR_TRACE, Log::Level::TRACE},
	{_STR_DEBUG, Log::Level::DEBUG},
	{_STR_INFO, Log::Level::INFO},
	{_STR_WARN, Log::Level::WARN},
	{_STR_ERROR, Log::Level::ERROR},
	{_STR_FATAL, Log::Level::FATAL},
};

static const std::map<Log::Level, const char*> s_lv2str = {
	{Log::Level::TRACE, _STR_TRACE},
	{Log::Level::DEBUG, _STR_DEBUG},
	{Log::Level::INFO, _STR_INFO},
	{Log::Level::WARN, _STR_WARN},
	{Log::Level::ERROR, _STR_ERROR},
	{Log::Level::FATAL, _STR_FATAL},
};

Log::Level
Log::s_toLevelType(const char* lv)
{
	auto ib(s_str2lv.find(std::string(lv)));
	if ( ib not_eq s_str2lv.end() ) return ib->second;

	switch(static_cast<Level>(atoi(lv)))
	{
	case Level::TRACE: return Level::TRACE;
	case Level::DEBUG: return Level::DEBUG;
	case Level::INFO: return Level::INFO;
	case Level::WARN: return Level::WARN;
	case Level::ERROR: return Level::ERROR;
	case Level::FATAL: return Level::FATAL;
	}

	return Level::TRACE;
}

const char*
Log::s_toString(Level lv)
{
	auto ib(s_lv2str.find(lv));
	if ( ib not_eq s_lv2str.end()) return ib->second;
	return _STR_TRACE;
}

void
Log::_s_LockTrace(const char* file, size_t line, const char* fmt, va_list ap)
{
	_DEF_BUF(buf);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	char ts[32];
	Log::s_makeTimeString(ts, time(nullptr));

	GLOBAL_LOCK();
	fprintf(stderr, "%s [%s:%zu] %s\r\n", ts, __getFilename(file), line, buf);
}

void
Log::s_trace(const char* file, size_t line, const char* fmt, ...)
{
	if ( s_use_trace )
	{
		va_list lst;
		va_start(lst, fmt);
		_s_LockTrace(file, line, fmt, lst);
		va_end(lst);
	}
}

void
Log::s_traceV(const char* file, size_t line, const char* fmt, va_list ap)
{
	if ( s_use_trace )
	{
		_s_LockTrace(file, line, fmt, ap);
	}
}

void
Log::s_abort(const char* file, size_t line, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	_s_LockTrace(file, line, fmt, lst);
	va_end(lst);
	abort();
}

void
Log::s_abortV(const char* file, size_t line, const char* fmt, va_list ap)
{
	_s_LockTrace(file, line, fmt, ap);
	abort();
}

void
Log::_s_LockLogLibrary(const char* file, size_t line, const char* fmt, va_list ap)
{
	_DEF_BUF(buf);
	ssize_t tlen(s_makeTimeString(buf, time(nullptr)));
	const ssize_t tslen(tlen);
	char* pbuf(&buf[tlen]);
	ssize_t plen(sizeof(buf)-tlen-2);
	tlen = snprintf(pbuf, plen, " [%s:%zu] ", __getFilename(file), line);

	pbuf += tlen;
	plen -= tlen;

	tlen = vsnprintf(pbuf, plen, fmt, ap);

	if ( tlen > plen ) tlen = plen;
	pbuf += tlen;
	plen -= tlen;
	memcpy(pbuf, "\r\n", 2);

	pbuf += 2;
	plen = ssize_t(pbuf - &(buf[0]));

	GLOBAL_LOCK();
	if ( s_log_library )
	{
		unique_lock_type local_locker(s_log_library->m_lock);
		s_log_library->_logRaw(buf, plen);
	}
	else if ( s_use_trace ) fwrite(buf+tslen+1, plen-tslen-1, 1, stderr);
}

void
Log::s_logLibrary(const char* file, size_t line, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	_s_LockLogLibrary(file, line, fmt, lst);
	va_end(lst);
}

void
Log::s_logLibraryV(const char* file, size_t line, const char* fmt, va_list ap)
{
	_s_LockLogLibrary(file, line, fmt, ap);
}

void
Log::s_setLevel(Level lv)
{
	GLOBAL_LOCK();

	Log* log(nullptr);
	for ( auto& iptr : s_logs )
	{
		log = reinterpret_cast<Log*>(iptr);

		unique_lock_type local_locker(log->m_lock);
		log->m_level = lv;
	}
}

Log::Log() : m_rotate_type(Rotate::DAILY), m_level(Level::ERROR), m_fd(-1), m_open(0), m_open_yday(-1), m_open_hour(-1)
{
	GLOBAL_LOCK();
	s_logs.insert(intptr_t(this));
}

Log::~Log()
{
	GLOBAL_LOCK();
	_close();

	s_logs.erase(intptr_t(this));
}

void
Log::swap(Log& v)
{
	std::lock(m_lock, v.m_lock);
	std::swap(m_rotate_type, v.m_rotate_type);
	std::swap(m_fd, v.m_fd);
	m_path.swap(v.m_path);
	m_prefix.swap(v.m_prefix);
	m_final_path.swap(v.m_final_path);
}

std::string&
Log::_generatePath(std::string& out, const std::string& path, const std::string& prefix, Rotate type)
{
	char _path[1024*10];
	ssize_t len(0);

	struct tm tm;
	time_t now(time(nullptr));
	localtime_r(&now, &tm);

	switch(type)
	{
	case Rotate::DAILY: len = snprintf(_path, sizeof(_path), "%s/%s%04d%02d%02d.log", path.c_str(), prefix.c_str(), tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday); break;
	case Rotate::HOURLY: len = snprintf(_path, sizeof(_path), "%s/%s%04d%02d%02d_%02d.log", path.c_str(), prefix.c_str(), tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour); break;
	}

	out.assign(_path, len);
	return out;
}

bool
Log::_open(const char* path, const char* prefix, Rotate type)
{
	std::string final_path;
	_generatePath(final_path, path, prefix, type);

	m_rotate_type = type;
	m_path = path;
	m_prefix = prefix;
	m_final_path = final_path;

	int fd(::open(final_path.c_str(), _PWLOG_OPENMODE, _PWLOG_OPENFLAG));
	if ( -1 == fd )
	{
		usleep(1000);
		fprintf(stderr, "failed to open file(%d): %s %s\n", errno, final_path.c_str(), strerror(errno) );
		return false;
	}

	this->_close();

	m_fd = fd;

	m_open = time(nullptr);
	struct tm tm;
	localtime_r(&m_open, &tm);
	m_open_hour = tm.tm_hour;
	m_open_yday = tm.tm_yday;

	_link();

	return true;
}

bool
Log::_checkTime(void) const
{
	time_t now(time(nullptr));
	struct tm tm;
	localtime_r(&now, &tm);
	if ( tm.tm_yday != m_open_yday ) return true;

	if ( m_rotate_type == Rotate::HOURLY )
	{
		if ( tm.tm_hour != m_open_hour ) return true;
	}

	return false;
}

void
Log::_logRaw(const char* s, size_t len)
{
	if ( -1 == m_fd ) _reopen();
	else if ( _checkTime() ) _reopen();

	if ( -1 not_eq m_fd )
	{
		if ( ::write(m_fd, s, len) < 0 )
		{
		}
		//::fdatasync(m_fd);
	}

	// not check global lock
	if ( s_use_trace )
	{
		// Standard Error
		if ( ::write(2, s, len) < 0 )
		{
		}
	}

}

void
Log::_logRawVector(const void* _in, size_t count)
{
	if ( -1 == m_fd ) _reopen();
	else if ( _checkTime() ) _reopen();

	auto v(reinterpret_cast<const struct iovec*>(_in));

	if ( -1 not_eq m_fd )
	{
		if ( ::writev(m_fd, v, count) < 0 )
		{
		}
		//::fdatasync(m_fd);
	}

	// not check global lock
	if ( s_use_trace )
	{
		// Standard Error
		if ( ::writev(2, v, count) < 0 )
		{
		}
	}

}

size_t
Log::s_makeTimeString(char* buf, time_t now)
{
	// YYYY-MM-DD HH:MM:SS ......
	struct tm tm;
	localtime_r(&now, &tm);
	return size_t(sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec));
}

void
Log::_LockLog(const char* fmt, va_list ap)
{
	_DEF_BUF(buf);
	char tmp_time[32];
	LOCAL_LOCK();
	_reopen();

	buf[0] = ' ';
	auto pbuf(&(buf[1]));
	size_t buflen( std::min( size_t(vsnprintf(pbuf, sizeof(buf)-1, fmt, ap)+1), sizeof(buf) ) );

	struct iovec iov[3];
	iov[0].iov_base = tmp_time;
	iov[0].iov_len = s_makeTimeString(tmp_time, time(nullptr));
	iov[1].iov_base = buf;
	iov[1].iov_len = buflen;
	iov[2].iov_base = &(s_lf[0]);
	iov[2].iov_len = 2;

	_logRawVector(iov, 3);
}

void
Log::log(const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	_LockLog(fmt, lst);
	va_end(lst);
}

void
Log::log(const char* fmt, va_list ap)
{
	_LockLog(fmt, ap);
}

void
Log::_LockLogLevel(Level level, const char* fmt, va_list ap)
{
	if ( m_level <= level )
	{
		_DEF_BUF(buf);
		char tmp_time[32];
		char tmp_time_level[64];

		s_makeTimeString(tmp_time, time(nullptr));

		LOCAL_LOCK();
		_reopen();

		buf[0] = ' ';
		auto pbuf(&(buf[1]));
		size_t buflen( std::min( size_t(vsnprintf(pbuf, sizeof(buf)-1, fmt, ap)+1), sizeof(buf) ) );

		struct iovec iov[5];
		iov[0].iov_base = tmp_time_level;
		iov[0].iov_len = snprintf(tmp_time_level, sizeof(tmp_time_level), "%s %s", tmp_time, s_toString(level));
		iov[1].iov_base = buf;
		iov[1].iov_len = buflen;
		iov[2].iov_base = &(s_lf[0]);
		iov[2].iov_len = 2;

		_logRawVector(iov, 3);
	}
}

void
Log::logLevel(Level level, const char* fmt, ...)
{
	va_list lst;
	va_start(lst, fmt);
	_LockLogLevel(level, fmt, lst);
	va_end(lst);
}

void
Log::logLevel(Level level, const char* fmt, va_list ap)
{
	_LockLogLevel(level, fmt, ap);
}

void
Log::_link ( void )
{
	if ( -1 == m_fd )
	{
		PWTRACE("not open file");
		return;
	}

	char front_path[1024*10];
	snprintf(front_path, sizeof(front_path), "%s/%slatest.log", m_path.c_str(), m_prefix.c_str());

	::unlink(front_path);
	if ( -1 == LINK(m_final_path.c_str(), front_path) )
	{
		PWTRACE("link failed: %s final:%s front:%s", strerror(errno), m_final_path.c_str(), front_path);
	}
}

bool
Log::_reopen(void)
{
	if ( m_fd == -1 )
	{
		//PWTRACE("open!?");
		return _open(m_path.c_str(), m_prefix.c_str(), m_rotate_type);
	}

	m_open = time(nullptr);
	struct tm tm;
	localtime_r(&m_open, &tm);

	if ( m_rotate_type == Rotate::DAILY )
	{
		if ( m_open_yday == tm.tm_yday ) return true;
	}
	else
	{
		if ( (m_open_hour == tm.tm_hour) and ( m_open_yday == tm.tm_yday ) ) return true;
	}

	std::string final_path;
	_generatePath(final_path, m_path, m_prefix, m_rotate_type);

	int fd(m_fd);
	m_fd = -1;
	::close(fd);

	//PWTRACE("open!! _reopen");
	if ( -1 == (m_fd = ::open(final_path.c_str(), _PWLOG_OPENMODE, _PWLOG_OPENFLAG) ) )
	{
		PWLOGLIB("failed to open log file: %s", final_path.c_str());
	}

	m_final_path = final_path;

	_link();

	m_open_hour = tm.tm_hour;
	m_open_yday = tm.tm_yday;

	return ( m_fd not_eq -1 );
}

bool
Log::_reopen(const char* path, const char* prefix, Rotate type)
{
	if ( (path[0] == 0x00) or (prefix[0] == 0x00) ) return false;

	if ( m_fd == -1 )
	{
		m_path = path;
		m_prefix = prefix;
		m_rotate_type = type;

		_reopen();
		return true;
	}

	bool do_reopen(false);
	if ( m_path.compare(path) ) { m_path.assign(path); do_reopen or_eq true; }
	if ( m_prefix.compare(prefix) ) { m_prefix.assign(prefix); do_reopen or_eq true; }
	if ( type not_eq m_rotate_type ) { m_rotate_type = type; do_reopen or_eq true; }

	if ( do_reopen )
	{
		//PWTRACE("do_reopen!");
		_close();
		_reopen();
	}

	return true;
}

};//namespace pw
