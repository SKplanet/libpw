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

#include "./pw_common.h"
#include "./pw_timer.h"
#include "./pw_ini.h"
#include "./pw_iopoller.h"
#include "./pw_listener_if.h"
#include "./pw_jobmanager.h"
#include "./pw_sysinfo.h"
#include "./pw_log.h"

#ifndef __PW_INSTANCE_H__
#define __PW_INSTANCE_H__

#ifndef pid_t
#	define pid_t	int
#endif

namespace pw {

//! \brief 인스턴스 인터페이스.
//	싱글톤 객체로 데몬마다 하나씩 생성한다.
//	각종 환경변수와 기본환경을 관리한다.
class InstanceInterface : public Timer::Event
{
public:
	// 아래 코드를 상속 받은 클래스에서 구현하세요.
	// inline static MyInstance& s_getInstance(void) { static MyInstance inst("my"); return inst; }

public:
	//! \brief 프로세스 타입
	enum class ProcessType
	{
		SINGLE,	//!< 싱글
		MULTI	//!< 멀티
	};

	//! \brief 차일드 프로세스 정보
	struct child_type final
	{
		size_t	index{static_cast<size_t>(-1)};		//!< 인덱스
		pid_t	pid{0};		//!< 차일드 프로세스 아이디
		void*	param{nullptr};		//!< 사용자 파라매터
		int		fd[2]{-1,-1};		//!< 페어 소켓

		inline const int& getFDByParent(void) const { return fd[0]; }
		inline int& getFDByParent(void) { return fd[0]; }
		inline const int& getFDByChild(void) const { return fd[1]; }
		inline int& getFDByChild(void) { return fd[1]; }
	};

	//! \brief 리스너 정보
	struct lsnr_type final
	{
		std::string port;
		ListenerInterface* lsnr { nullptr };
	};

	using lsnr_cont = std::map<std::string, lsnr_type>;
	using lsnr_names = std::set<std::string>;

public:
	//! \brief 각종 라이브러리 초기화
	static bool s_initLibraries(void);

	//! \brief 각종 시그널 초기화
	static bool s_initSignals(void);

public:
	//! \brief 생성자.
	explicit InstanceInterface(const char* apptag);

	//! \brief 소멸자.
	virtual ~InstanceInterface();

public:
	//! \brief 인스턴스 시작.
	int start(int argc, char* argv[]);

	//! \brief 환경설정 읽기.
	bool loadConfig(const char* path, bool isDefault, bool isReload);

	//! \brief 로그 설정 초기화
	//! \param[out] log 로그 객체
	//! \param[in] ini 설정
	//! \param[in] typetag 타입태그. 로그파일명을 결정한다. 예) cmd, err
	//! \param[in] ini_item 설정 아이템 이름.
	//! \param[in] ini_section 설정 섹션 이름.
	bool reloadLog(Log& log, const Ini& ini, const std::string& typetag, const std::string& ini_item, const std::string& ini_section);

	//! \brief 로그 초기화
	bool initializeLog(Log& log, const char* typetag, const char* path, Log::Rotate rtype = Log::Rotate::DAILY);

	char* makeLogPrefix(char* obuf, size_t obuflen, const char* typetag) const;
	std::string& makeLogPrefix(std::string& obuf, const std::string& typetag) const;

	//! \brief 리스너 객체 가져오기
	inline const ListenerInterface* getListener(const std::string& name) const { auto ib(m_lsnrs.find(name)); return (ib not_eq m_lsnrs.end() ? ib->second.lsnr : nullptr); }
	inline ListenerInterface* getListener(const std::string& name) { auto ib(m_lsnrs.find(name)); return (ib not_eq m_lsnrs.end() ? ib->second.lsnr : nullptr); }

	//! \brief 리스너 정보 객체 가져오기
	//inline const lsnr_type& getListenerInfo(const std::string& name) const { return m_lsnrs[name]; }
	inline lsnr_type& getListenerInfo(const std::string& name) { return m_lsnrs[name]; }
	lsnr_type* getListenerInfo(const ListenerInterface* lsnr, const lsnr_type* e = nullptr);
	const lsnr_type* getListenerInfo(const ListenerInterface* lsnr, const lsnr_type* e = nullptr) const;

	bool loadPort(const std::string& tag, const std::string& sec);
	bool loadPort(const std::string& tag, Ini::sec_citr sec);

	//! \brief 해당 리스너를 가진 리스너정보에 리스너를 널포인트로 만듦.
	void setListenerEmpty(const ListenerInterface* lsnr);

	//! \brief Fork
	bool fork(size_t index, void* param);

	//! \brief 폴러를 강제로 반환.
	bool wakeUp(void);

	//! \brief 차일드 프로세스인지 확인
	inline bool isChild(void) const { return m_child.index not_eq size_t(-1); }

	//! \brief 차일드 인덱스 가져오기
	inline size_t getChildIndex(void) const { return m_child.index; }

	//! \brief 차일드 개수 가져오기
	inline size_t getChildCount(void) const { return m_child.count; }

	//! \brief 차일드에게 시그널 보내기
	//! \warning 호출한 쪽이 차일드이거나, 차일드가 없는 환경이면 무시한다.
	bool signalToChild(int signal, size_t idx) const;

	//! \brief 모든 차일드에게 시그널 보내기
	//! \warning 호출한 쪽이 차일드이거나, 차일드가 없는 환경이면 무시한다.
	bool signalToChildAll(int signal) const;

	bool signalToChild_Interrupt(size_t idx) const;
	bool signalToChildAll_Interrupt(void) const;
	bool signalToChild_User1(size_t idx) const;
	bool signalToChildAll_User1(void) const;
	bool signalToChild_User2(size_t idx) const;
	bool signalToChildAll_User2(void) const;
	bool signalToChild_Kill(size_t idx) const;
	bool signalToChildAll_Kill(void) const;

	void logError(const char* file, size_t line, const char* fmt, ...) __attribute__((format(printf,4,5)));
	void logErrorV(const char* file, size_t line, const char* fmt, va_list ap);

	//! \brief 이름기반으로 리스너에 사용할 SslContext 가져오기
	virtual SslContext* getListenSslContext(const std::string& name) const { return nullptr; }

	//! \brief 싱글 모드 리스너 열기 템플릿 함수
	template<typename _ListenerType>
	bool openListenerSingle(const std::string& name);

	//! \brief 멀티 모드 부모 리스너 열기 템플릿 함수
	template<int _ListenerType>
	bool openListenerParent(const std::string& name);

	//! \brief 멀티 모드 자식 리스너 열기 템플릿 함수
	template<typename _ListenerType>
	bool openListenerChild(void);

	//! \brief 멀티 모드 자식 리스너 열기 템플릿 함수
	template<typename _ListenerType>
	bool openListenerChild(const lsnr_names& names);

	//! \brief 싱글 프로세스 타입 확인
	inline bool isSingle(void) const { if ( isChild() ) return false; return ( (m_child.type == ProcessType::SINGLE) or (m_child.count == 0) ); }

	//! \brief 워크 프로세스인지 확인
	inline virtual bool isWorkProcess(void) const { if ( isChild() ) return true; return (m_child.count == 0); }

	//! \brief 프로세스 타입 확인
	inline ProcessType getProcessType(void) const { return m_child.type; }

	//! \brief 인덱스로 차일드 PID
	//! \return 실패하면 pid_t(-1)을 반환한다.
	inline pid_t getChildPID(size_t index) const
	{
		const child_type* ci(getChildByIndex(index));
		return ci == nullptr ? pid_t(-1) : ci->pid;
	}

	//! \brief PID로 차일드 인덱스
	//! \return 실패하면 size_t(-1)을 반환한다.
	inline size_t getChildIndexByPID(pid_t pid) const
	{
		for ( size_t i(0); i < m_child.count; i++ )
		{
			if ( m_child.cont[i].pid == pid ) return i;
		}

		return size_t(-1);
	}

	//! \brief PID로 차일드 객체
	//! \return 실패하면 nullptr을 반환한다.
	inline child_type* getChildByPID(pid_t pid)
	{
		for ( size_t i(0); i < m_child.count; i++ )
		{
			if ( m_child.cont[i].pid == pid ) return &(m_child.cont[i]);
		}

		return nullptr;
	}

	//! \brief PID로 차일드 객체
	//! \return 실패하면 nullptr을 반환한다.
	inline const child_type* getChildByPID(pid_t pid) const
	{
		for ( size_t i(0); i < m_child.count; i++ )
		{
			if ( m_child.cont[i].pid == pid ) return &(m_child.cont[i]);
		}

		return nullptr;
	}

	//! \brief 인덱스로 차일드 객체
	//! \return 실패하면 nullptr을 반환한다.
	inline child_type* getChildByIndex(size_t index)
	{
		return ( index >= m_child.count ) ? nullptr : &(m_child.cont[index]);
	}

	//! \brief 인덱스로 차일드 객체
	//! \return 실패하면 nullptr을 반환한다.
	inline const child_type* getChildByIndex(size_t index) const
	{
		return ( index >= m_child.count ) ? nullptr : &(m_child.cont[index]);
	}

	//! \brief 차일드 자신의 객체
	//! \return 실패하면 nullptr을 반환한다.
	inline child_type* getChildSelf(void)
	{
		return ( size_t(-1) == m_child.index ) ? nullptr : &(m_child.cont[m_child.index]);
	}

	//! \brief 차일드 자신의 객체
	//! \return 실패하면 nullptr을 반환한다.
	const child_type* getChildSelf(void) const
	{
		return ( size_t(-1) == m_child.index ) ? nullptr : &(m_child.cont[m_child.index]);
	}

	//! \brief 환경설정 객체
	const Ini& getConfig(void) const { return m_config.conf; }

	//! \brief 죽은 차일드 개수
	//!	보통 데몬 라이프 타임동안 누적한다.
	inline size_t getChildDeadCount(void) const { return m_child.dead_count; }

	//! \brief 눅은 차일드 개수를 리셋한다.
	inline void resetChildDeadCount(void) { m_child.dead_count = 0; }

protected:
	//! \brief 기본 환경설정 읽기.
	bool loadDefaultConfig(bool isReload);

	//! \brief fork한 뒤, 리소스 정리
	void cleanUpForChild(size_t index, void* param);

protected:
	//! \brief 환경설정 읽기.
	//! \param[in] isDefault 데몬 초기화 시 최초 읽는 순간인지 여부.
	//! \param[in] isReload 데몬 가동 중 다시 읽기인지 여부.
	inline virtual bool eventConfig(bool isDefault, bool isReload) { return true; }

	//! \brief 로그 초기화.
	inline virtual bool eventInitLog(void) { return true; }

	//! \brief (서비스)채널 초기화
	inline virtual bool eventInitChannel(void) { return true; }

	//! \brief 리스너 초기화
	inline virtual bool eventInitListener(void)
	{
		if ( isSingle() ) return eventInitListenerSingle();
		if ( isChild() ) return eventInitListenerChild();
		return eventInitListenerParent();
	}
	inline virtual bool eventInitListenerSingle(void) { return true; }
	inline virtual bool eventInitListenerChild(void) { return true; }
	inline virtual bool eventInitListenerParent(void) { return true; }

	//! \brief 타이머 초기화
	inline virtual bool eventInitTimer(void) { return true; }

	//! \brief 기타 초기화
	inline virtual bool eventInitExtras(void) { return true; }

	//! \brief 차일드 초기화
	virtual bool eventInitChild(void);

	//! \brief 데몬 종료시 호출함
	virtual void eventExit(void);

	//! \brief 차일드 종료시 부모에서 호출함
	virtual void eventExitChild(size_t index, pid_t pid, int exit_status, void* param);

	//! \brief 매 poller 턴이 끝날 때마다 호출함
	inline virtual void eventEndTurn(void) {};

	//! \brief fork한 뒤, 차일드에서 호출함
	virtual void eventForkChild(size_t index, void* param);

	//! \brief fork한 뒤, 차일드에서 (서비스)채널 정리
	inline virtual void eventForkCleanUpChannel(size_t index, void* param) {}

	//! \brief fork한 뒤, 차일드에서 리스너 객체 정리
	virtual void eventForkCleanUpListener(size_t index, void* param);

	//! \brief fork한 뒤, 차일드에서 타이머 객체 정리
	virtual void eventForkCleanUpTimer(size_t index, void* param);

	//! \brief fork한 뒤, 차일드에서 기타 정리
	inline virtual void eventForkCleanUpExtras(size_t index, void* param) {}

	//! \brief fork한 뒤, 차일드에서 poller 정리
	virtual void eventForkCleanUpPoller(size_t index, void* param);

public:
	//! \brief 데몬 태그
	//!	예) dispatcher
	inline const char* getAppTag(void) const { return m_app.tag.c_str(); }

	//! \brief 데몬 이름
	//!	예) dispatcher01
	inline const char* getAppName(void) const { return m_app.name.c_str(); }

	//! \brief 실행 플래그.
	//!	false이면 인스턴스 루프를 종료한다.
	inline bool getFlagRun(void) const { return m_flag.run; }

	//! \brief 환경설정 다시 읽기 플래그.
	inline bool getFlagReload(void) const { return m_flag.reload; }

	//! \brief 스테이지 여부.
	inline bool getFlagStage(void) const { return m_flag.stage; }

	//! \brief 실행 인자 개수.
	inline int getArgsCount(void) const { return m_args.count; }

	//! \brief 실행 인자 문자열 배열.
	inline char** const getArgsValue(void) const { return m_args.value; }

	//! \brief 실행 인자 문자열.
	inline const char* getArgsValue(size_t index) const
	{
		if ( index < size_t(m_args.count) ) return m_args.value[index];
		return nullptr;
	}

	//! \brief Job 타임아웃
	inline int64_t getTimeoutJob(void) const { return m_timeout.job; }

	//! \brief Ping 타임아웃
	inline int64_t getTimeoutPing(void) const { return m_timeout.ping; }

	//! \brief 실행 플래그 설정.
	inline void setFlagRun(bool run) { m_flag.run = run; }

	//! \brief 실행 플래그를 거짓으로 세팅하고, 종료코드도 설정한다.
	inline void setFlagStop(int exit_code) { m_exit_code = exit_code; m_flag.run = false; }

	//! \brief 환경설정 다시 읽기 플래그 설정.
	inline void setFlagReload(bool reload = true) { m_flag.reload = reload; }

	//! \brief 스테이지 여부 설정.
	inline void setFlagStage(bool stage) { m_flag.stage = stage; }

	//! \brief 차일드 프로세스 검사 설정.
	inline void setFlagCheckChild(bool check_child = true) { m_flag.check_child = check_child; }

	//! \brief 폴러 타입.
	inline const char* getPollerType(void) const { return m_poller.type.c_str(); }

	//! \brief 폴러 객체.
	inline IoPoller* getPoller(void) { return m_poller.poller; }

	//! \brief 폴러 객체.
	inline const IoPoller* getPoller(void) const { return m_poller.poller; }

	inline int64_t getPollerTimeout(void) const { return m_poller.timeout; }

	//! \brief 시작 시간
	inline time_t getStart(void) const { return m_start.child; }

	//! \brief 부모 시작 시간
	inline time_t getStartParent(void) const { return m_start.parent; }

	//! \brief 빌드 시간
	inline time_t getStartBuild(void) const { return m_start.build; }

	//! \brief 빌드 시간 세팅
	void setStartBuild(const char* build_date, const char* build_time);

	//! \brief 종료코드 얻기
	inline int getExitCode(void) const { return m_exit_code; }

	//! \brief 종료코드 설정
	inline void setExitCode(int code) { m_exit_code = code; }

private:
	void _checkChild(void);
	void _closePair(int fd[2]);
	bool _initChildInfo(void);

public:
	//! \brief 싱글톤 객체.
	static InstanceInterface*	s_inst;

	struct {
		std::string	path;	//!< 위치
		Ini			conf;	//!< 파서
	} m_config;		//!< 환경설정

	SystemInformation	m_sysinfo;

	struct {
		Log			cmd;		//!< 커맨드 로그 객체
		Log			err;		//!< 오류 로그 객체
	} m_log;		//!< 로그 설정

	lsnr_cont	m_lsnrs;		//!< 리스너 컨테이너

	struct {
		int64_t			job;	//!< 잡 타임아웃
		int64_t			ping;	//!< 핑 타임아웃
	} m_timeout;	//!< 타임아웃

	struct {
		JobManager	man;	//!< 잡 매니저.
	} m_job;

private:
	struct {
		std::string		tag;	//!< 서버 종류.
		std::string		name;	//!< 서버 개별 이름.
	} m_app;		//!< 기본 앱 정보

	struct {
		std::string		type;	//!< 타입
		IoPoller*		poller;	//!< 객체
		int64_t			timeout;//!< 타임아웃
	} m_poller;		//!< 폴러 정보

	struct {
		bool run;		//!< 실행 루프
		bool reload;	//!< 환경설정 다시 읽기
		bool stage;		//!< 스테이지 여부
		bool check_child;	//!< 차일드가 죽었는지 확인
	} m_flag;		//!< 일반적인 플래그

	struct {
		int		count;	//!< int argc
		char**	value;	//!< char* argv[]
	} m_args;		//!< 실행 인자

	struct {
		ProcessType	type;	//!< 멀티프로세스 사용여부
		size_t			count;	//!< 멀티프로세스 개수
		size_t			dead_count;	//!< 죽은 차일드 개수
		size_t			index;	//!< 차일드 인덱스. 부모일 경우 -1
		child_type*		cont;	//!< 차일드 객체
	} m_child;		//!< 차일드 설정

	const struct _start {
		time_t			build;	//!< 빌드 시간
		time_t			parent;	//!< 부모 시작 시간
		time_t			child;	//!< 자식 시작 시간

		inline _start() : build(0), parent(0), child(0) {}
	} m_start;		//!< 시작 시간

	int m_exit_code;	//!< 프로세스 종료 코드

private:
	class _WakeUp final: public IoPoller::Event
	{
	public:
		explicit _WakeUp(InstanceInterface& inst);
		virtual ~_WakeUp();

		void reopen(void);
		bool setEventOut(void);

	private:
		void eventIo(int, int, bool&);

	private:
		InstanceInterface& m_inst;
		int m_fd;

	friend class InstanceInterface;
	} m_wakeup;	//!< 폴러 강제 종료 객체
};

template<typename _ListenerType>
bool
InstanceInterface::openListenerSingle (const std::string& name)
{
	auto& ctx(this->m_lsnrs[name]);
	auto sslctx(this->getListenSslContext(name));
	auto& plsnr(ctx.lsnr);
	do {
		if ( plsnr ) { plsnr->close(); delete plsnr; plsnr = nullptr; }
		if ( nullptr == ( plsnr = new _ListenerType(this->getPoller()) ) )
		{
			this->logError(__FILE__, __LINE__, "failed to create listener: type:%s name:%s", typeid(_ListenerType).name(), name.c_str());
			break;
		}

		plsnr->setSslContext(sslctx);

		SocketAddress sa;
		sa.setIP4("0", ctx.port.c_str());
		if ( not plsnr->open(sa) )
		{
			this->logError(__FILE__, __LINE__, "failed to open listener: type:%s name:%s port:%s", typeid(_ListenerType).name(), name.c_str(), ctx.port.c_str());
			break;
		}

		return true;
	} while (false);

	if ( plsnr ) { plsnr->close(); delete plsnr; plsnr = nullptr; }
	return false;
}

template<int _ListenerType>
bool
InstanceInterface::openListenerParent(const std::string& name)
{
	auto& ctx(this->m_lsnrs[name]);
	auto& plsnr(ctx.lsnr);
	do {
		if ( plsnr ) { plsnr->close(); delete plsnr; plsnr = nullptr; }
		if ( nullptr == ( plsnr = new pw::ParentListener(_ListenerType, this->getPoller()) ) )
		{
			this->logError(__FILE__, __LINE__, "failed to create parent listener: type:%d name:%s", _ListenerType, name.c_str());
			break;
		}

		SocketAddress sa;
		sa.setIP4("0", ctx.port.c_str());
		if ( not plsnr->open(sa) )
		{
			this->logError(__FILE__, __LINE__, "failed to open parent listener: type:%d name:%s port:%s", _ListenerType, name.c_str(), ctx.port.c_str());
			break;
		}

		return true;
	} while (false);

	if ( plsnr ) { plsnr->close(); delete plsnr; plsnr = nullptr; }
	return false;
}

//! \brief 멀티 모드 자식 리스너 열기 템플릿 함수
template<typename _ListenerType>
bool
InstanceInterface::openListenerChild(void)
{
	if ( m_lsnrs.empty() )
	{
		this->logError(__FILE__, __LINE__, "no listener names. use InstanceInterface::openListenerChild(const lsnr_names& names) method.");
		return false;
	}

	auto plsnr(new _ListenerType(this->getPoller()));
	if ( nullptr == plsnr )
	{
		this->logError(__FILE__, __LINE__, "failed to create child listener");
		return false;
	}

	if ( not plsnr->open() )
	{
		plsnr->close();
		delete plsnr;
		return false;
	}

	for ( auto& ctx : m_lsnrs )
	{
		auto& clsnr(ctx.second.lsnr);
		if ( clsnr ) { clsnr->close(); delete clsnr; }

		clsnr = plsnr;
	}

	return true;
}

//! \brief 멀티 모드 자식 리스너 열기 템플릿 함수
template<typename _ListenerType>
bool
InstanceInterface::openListenerChild(const lsnr_names& names)
{
	if ( names.empty() )
	{
		this->logError(__FILE__, __LINE__, "no listener names");
		return false;
	}

	auto plsnr(new _ListenerType(this->getPoller()));
	if ( nullptr == plsnr )
	{
		this->logError(__FILE__, __LINE__, "failed to create child listener");
		return false;
	}

	if ( not plsnr->open() )
	{
		plsnr->close();
		delete plsnr;
		return false;
	}

	for ( auto& name : names )
	{
		auto& clsnr(this->m_lsnrs[name].lsnr);
		if ( clsnr ) { clsnr->close(); delete clsnr; }

		clsnr = plsnr;
	}

	return true;
}

};//namespace pw
#endif//!__PW_INSTANCE_H__

class s_initLibraries;
