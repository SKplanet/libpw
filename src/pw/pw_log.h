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
 * \file pw_log.h
 * \brief Simple log system.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_LOG_H__
#define __PW_LOG_H__

namespace pw {

//! \brief 로그 클래스
class Log final
{
public:
	//! \brief 순환 형태
	enum class Rotate
	{
		DAILY,	//!< 일 단위 생성
		HOURLY	//!< 시간 단위 생성
	};

	//! \brief 로그 레벨
	enum class Level
	{
		TRACE = 0,	//!< 트레이스
		DEBUG,		//!< 디버그
		INFO,		//!< 정보
		WARN,		//!< 경고
		ERROR,		//!< 오류
		FATAL,		//!< 심각
	};

	using lock_type = std::mutex;
	using unique_lock_type = std::unique_lock<lock_type>;

public:
	//! \brief 트래이스 로그. PWTRACE() 매크로를 사용할 것.
	static void s_trace(const char* file, size_t line, const char* fmt, ...) __attribute__((format(printf,3,4)));
	static void s_traceV(const char* file, size_t line, const char* fmt, va_list ap);

	//! \brief 중단로그. 무조건 표준오류에 출력하고, abort()를 호출한다. PWABORT) 매크로를 사용할 것.
	static void s_abort(const char* file, size_t len, const char* fmt, ...) __attribute__((format(printf,3,4)));
	static void s_abortV(const char* file, size_t len, const char* fmt, va_list ap);

	//! \brief 라이브러리 로그. pw::Log::s_log_library를 설정하지 않으면 표준오류로 출력한다.
	static void s_logLibrary(const char* file, size_t line, const char* fmt, ...) __attribute__((format(printf,3,4)));
	static void s_logLibraryV(const char* file, size_t line, const char* fmt, va_list ap);

	//! \brief 타임스트링
	static size_t s_makeTimeString(char* buf, time_t now);

	//! \brief 라이브러리 로그 설정
	inline static void s_setLibrary(Log* p) { unique_lock_type locker(s_lock); s_log_library = p; }

	//! \brief 모든 라이브러리 레벨 변경
	void s_setLevel(Level lv);

	//! \brief 모든 라이브러리 레벨 변경
	inline void s_setLevel(const char* lv) { s_setLevel(s_toLevelType(lv)); }

	//! \brief 문자열을 로그레벨로 변경한다.
	static Level s_toLevelType(const char* lv);

	//! \brief 로그레벨을 문자열로 변경한다.
	static const char* s_toString(Level lv);

	static void s_setTrace(bool use) { s_use_trace = use; }
	static bool s_getTrace(void) { return s_use_trace; }

public:
	explicit Log();
	virtual ~Log();

	Log(const Log&) = delete;
	Log(Log&&) = delete;
	Log& operator = (const Log&) = delete;
	Log& operator = (Log&&) = delete;

	inline void reopen(void) { unique_lock_type locker(m_lock); _reopen(); }
	inline bool reopen(const char* path, const char* prefix, Rotate type) { unique_lock_type locker(m_lock); return _reopen(path, prefix, type); }
	inline bool open(const char* path, const char* prefix, Rotate type) { unique_lock_type locker(m_lock); return _open(path, prefix, type); }
	inline void close(void) { unique_lock_type locker(m_lock); _close(); }

	inline void getPath(std::string& out) const { unique_lock_type locker(m_lock); out = m_path; }
	inline void getPrefix(std::string& out) const { unique_lock_type locker(m_lock); out = m_prefix; }
	inline void getFinalPath(std::string& out) const { unique_lock_type locker(m_lock); out = m_final_path; }

	void swap(Log& v);

	void log(const char* fmt, ...) __attribute__((format(printf,2,3)));
	void log(const char* fmt, va_list ap);
	void logLevel(Level lv, const char* fmt, ...) __attribute__((format(printf,3,4)));
	void logLevel(Level lv, const char* fmt, va_list ap);
	bool checkTime(void) const { unique_lock_type locker(m_lock); return _checkTime(); }

	inline Level getLevel(void) const { return m_level; }
	inline void setLevel(Level lv) { unique_lock_type locker(m_lock); m_level = lv; }
	inline void setLevel(const std::string& s) { this->setLevel(s_toLevelType(s.c_str())); }
	inline bool isLevelOver(Level lv) { return m_level <= lv; }

	inline Rotate getRotate(void) const { return m_rotate_type; }
	inline void setRotate(Rotate r) { unique_lock_type locker(m_lock); m_rotate_type = r; }

private:
	static bool	s_use_trace;
	static Log*	s_log_library;

protected:
	std::string& _generatePath(std::string& out, const std::string& path, const std::string& prefix, Rotate rotate_type);
	void _logRaw(const char* s, size_t len);
	void _logRawVector(const void* v, size_t count);
	void _close(void) { if ( m_fd ) {::close(m_fd); m_fd = -1; } }
	bool _reopen(void);
	bool _reopen(const char* path, const char* prefix, Rotate type);
	bool _open(const char* path, const char* prefix, Rotate type);
	bool _checkTime(void) const;
	void _link(void);

	static void _s_LockTrace(const char* file, size_t line, const char* fmt, va_list ap);
	static void _s_LockLogLibrary(const char* file, size_t line, const char* fmt, va_list ap);
	void _LockLog(const char* fmt, va_list ap);
	void _LockLogLevel(Level lv, const char* fmt, va_list ap);

protected:
	Rotate		m_rotate_type;
	Level		m_level;
	int			m_fd;
	std::string	m_path;
	std::string	m_prefix;
	std::string	m_final_path;

	time_t		m_open;
	int			m_open_yday;
	int			m_open_hour;

	mutable std::mutex	m_lock;

private:
	static std::set<intptr_t>	s_logs;
	static std::mutex			s_lock;

	friend class InstanceInterface;
};

//! \brief 트레이스 로그
#define PWTRACE(fmt,...) pw::Log::s_trace(__FILE__, __LINE__, fmt, ##__VA_ARGS__ )
#ifdef _PW_HEAVY_TRACE
#	define PWTRACE_HEAVY(fmt,...) pw::Log::s_trace(__FILE__, __LINE__, fmt, ##__VA_ARGS__ )
#else
#	define PWTRACE_HEAVY(fmt,...) do {} while (false)
#endif

//! \brief 어보트 로그
//! \warning abort()를 호출하므로, 코어를 남기고 종료한다.
#define PWABORT(fmt,...) pw::Log::s_abort(__FILE__, __LINE__, fmt, ##__VA_ARGS__ )

//! \brief 라이브러리 로그
#define PWLOGLIB(fmt,...) pw::Log::s_logLibrary(__FILE__, __LINE__, fmt, ##__VA_ARGS__ )

#if (NDEBUG)
#	define PWSHOWFUNC() do {} while (false)
#	define PWSHOWMETHOD() do {} while (false)
#else
	//! \brief 함수 로그
#	define PWSHOWFUNC() PWTRACE("%s", __func__)
	//! \brief 메소드 로그
#	define PWSHOWMETHOD() PWTRACE("%s(this:%p %s)", __func__, this, typeid(*this).name())
#endif

//! \brief 트래이스 레벨 로그
#define PWLOGTRACE(log,fmt,...) (log).logLevel(pw::Log::Level::TRACE, fmt, ##__VA_ARGS__)
//! \brief 디버그 레벨 로그
#define PWLOGDEBUG(log,fmt,...) (log).logLevel(pw::Log::Level::DEBUG, fmt, ##__VA_ARGS__)
//! \brief 인포 레벨 로그
#define PWLOGINFO(log,fmt,...) (log).logLevel(pw::Log::Level::INFO, fmt, ##__VA_ARGS__)
//! \brief 경고 레벨 로그
#define PWLOGWARN(log,fmt,...) (log).logLevel(pw::Log::Level::WARN, fmt, ##__VA_ARGS__)
//! \brief 오류 레벨 로그
#define PWLOGERROR(log,fmt,...) (log).logLevel(pw::Log::Level::ERROR, fmt, ##__VA_ARGS__)
//! \brief 심각 레벨 로그
#define PWLOGFATAL(log,fmt,...) (log).logLevel(pw::Log::Level::FATAL, fmt, ##__VA_ARGS__)

template<typename _Type>
inline
const char*
cstr(const _Type& s)
{
	return s.c_str();
}

}; //namespace pw

#endif//!__PW_LOG_H__
