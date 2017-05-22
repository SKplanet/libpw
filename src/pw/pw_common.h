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
 * \file pw_common.h
 * \brief Library common settings.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 * \warning Do not use this file directly.
 */

#include "./pwlib_config.h"

#ifndef __PW_COMMON_H__
#define __PW_COMMON_H__

//==============================================================================
// 기본 헤더 파일 섹션
// C++ standard
// Support stream
#include <iostream>
#include <fstream>
#include <sstream>

// Containers
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <forward_list>
#include <vector>
#include <queue>
#include <stack>
#include <typeinfo>
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>

// Thread
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

// Time
#include <chrono>

// Locale
#include <string>
#include <locale>
#include <regex>

// C standard by C++
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <cinttypes>
#include <ciso646>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace std;

// UNIX standard
#include <unistd.h>
#include <sys/socket.h>

#ifndef HAVE_SOCKLEN_T
#	define socklen_t	uint32_t
#endif//HAVE_SOCKLEN_T

// GCC >= 4.x
#define PWEXPORT __attribute__((visibility("default")))
#define PWIMPORT __attribute__((visibility("default")))
#define PWLOCAL __attribute__((visibility("hidden")))
#define PWDEPRECATED __attribute__((deprecated))
#define PWAPI PWEXPORT

//==============================================================================
// 매드 네임 스페이스 섹션
//! \namespace pw
//! \brief 기본 매드 라이브러리 네임 스페이스
namespace pw {

using init_func_type = bool (*) (void);

extern bool _PWINIT(void);
extern void _PWINIT_ADD(init_func_type);
#define PWINIT()		pw::_PWINIT()
#define PWINIT_ADD(x)	pw::_PWINIT_ADD(x)

//! \brief 응답코드. HTTP 응답코드와 호환한다.
enum class ResultCode : int
{
	EMPTY = 0, //!< 초기값.

	// ... 1xx
	CONTINUE = 100,
	SWITCHING_PROTOCOL = 101,

	// Success 2xx
	SUCCESS = 200,	//!< 성공
	CREATED = 201,
	ACCEPTED = 202,
	NOAUTH_INFORMATION = 203,
	NO_CONTENT = 204,
	RESET_CONTENT = 205,
	PARTIAL_CONTENT = 206,

	// Redirection 3xx
	MULTIPLE_CHOICES = 300,
	MOVED_PERMANENTLY = 301,	//!< 브릿지 페이지 이동
	FOUND = 302,
	SEE_OTHER = 303,
	NOT_MODIFIED = 304,
	USE_PROXY = 305,
	TEMPORARY_REDIRECT = 307,

	// Client Error 4xx
	BAD_REQUEST = 400,	//!< 잘못된 인자값
	UNAUTHORIZED = 401,	//!< 인증 실패
	PAYMENT_REQUIRED = 402,	//!< 유료 서비스
	FORBIDDEN = 403,	//!< 접근 금지
	NOT_FOUND = 404,	//!< 알 수 없는 코드
	METHOD_NOT_ALLOWED = 405,
	NOT_ACCEPTABLE = 406,
	PROXY_AUTH_REQUIRED = 407,
	REQUEST_TIMEOUT = 408,	//!< 타임아웃
	CONFLICT = 409,
	GONE = 410,	//!< 서버 오류
	LENGTH_REQUIRED = 411,
	PRECONDITION_FAILED = 412,
	REQUEST_ENTITY_TOO_LARGE = 413, //!< 각 필드가 너무 길 때
	REQUEST_URI_TOO_LONG = 414,	//!< 패킷이 너무 클 때
	UNSUPPORTED_MEDIA_TYPE = 415,	//!< 지원하지 않는 미디어
	REQUEST_RANGE_FAILED = 416,	//!< 요청 범위를 만족시킬 수 없을 때
	EXPECT_FAILED = 417,

	// Server Error 5xx
	INTERNAL_SERVER_ERROR = 500,	//!< 알 수 없는 서버 오류
	NOT_IMPLEMENTED = 501,	//!< 구현하지 않음
	BAD_GATEWAY = 502,
	SERVICE_UNAVAILABLE = 503,	//!< 서비스 사용 불가
	GATEWAY_TIMEOUT = 504,
	VERSION_NOT_SUPPORTED = 505, //!< 지원하지 않는 버전
};

#define PWRES_CODE_EMPTY					pw::ResultCode::EMPTY
#define PWRES_CODE_CONTINUE					pw::ResultCode::CONTINUE
#define PWRES_CODE_SWITCHING_PROTOCOL		pw::ResultCode::SWITCHING_PROTOCOL
#define PWRES_CODE_SUCCESS					pw::ResultCode::SUCCESS
#define PWRES_CODE_CREATED					pw::ResultCode::CREATED
#define PWRES_CODE_ACCEPTED					pw::ResultCode::ACCEPTED
#define PWRES_CODE_NOAUTH_INFORMATION		pw::ResultCode::NOAUTH_INFORMATION
#define PWRES_CODE_NO_CONTENT				pw::ResultCode::NO_CONTENT
#define PWRES_CODE_RESET_CONTENT			pw::ResultCode::RESET_CONTENT
#define PWRES_CODE_PARTIAL_CONTENT			pw::ResultCode::PARTIAL_CONTENT
#define PWRES_CODE_MULTIPLE_CHOICES			pw::ResultCode::MULTIPLE_CHOICES
#define PWRES_CODE_MOVED_PERMANENTLY		pw::ResultCode::MOVED_PERMANENTLY
#define PWRES_CODE_FOUND					pw::ResultCode::FOUND
#define PWRES_CODE_SEE_OTHER				pw::ResultCode::SEE_OTHER
#define PWRES_CODE_NOT_MODIFIED				pw::ResultCode::NOT_MODIFIED
#define PWRES_CODE_USE_PROXY				pw::ResultCode::USE_PROXY
#define PWRES_CODE_TEMPORARY_REDIRECT		pw::ResultCode::TEMPORARY_REDIRECT
#define PWRES_CODE_BAD_REQUEST				pw::ResultCode::BAD_REQUEST
#define PWRES_CODE_UNAUTHORIZED				pw::ResultCode::UNAUTHORIZED
#define PWRES_CODE_PAYMENT_REQUIRED			pw::ResultCode::PAYMENT_REQUIRED
#define PWRES_CODE_FORBIDDEN				pw::ResultCode::FORBIDDEN
#define PWRES_CODE_NOT_FOUND				pw::ResultCode::NOT_FOUND
#define PWRES_CODE_METHOD_NOT_ALLOWED		pw::ResultCode::METHOD_NOT_ALLOWED
#define PWRES_CODE_NOT_ACCEPTABLE			pw::ResultCode::NOT_ACCEPTABLE
#define PWRES_CODE_PROXY_AUTH_REQUIRED		pw::ResultCode::PROXY_AUTH_REQUIRED
#define PWRES_CODE_REQUEST_TIMEOUT			pw::ResultCode::REQUEST_TIMEOUT
#define PWRES_CODE_CONFLICT					pw::ResultCode::CONFLICT
#define PWRES_CODE_GONE						pw::ResultCode::GONE
#define PWRES_CODE_LENGTH_REQUIRED			pw::ResultCode::LENGTH_REQUIRED
#define PWRES_CODE_PRECONDITION_FAILED		pw::ResultCode::PRECONDITION_FAILED
#define PWRES_CODE_REQUEST_ENTITY_TOO_LARGE	pw::ResultCode::REQUEST_ENTITY_TOO_LARGE
#define PWRES_CODE_REQUEST_URI_TOO_LONG		pw::ResultCode::REQUEST_URI_TOO_LONG
#define PWRES_CODE_UNSUPPORTED_MEDIA_TYPE	pw::ResultCode::UNSUPPORTED_MEDIA_TYPE
#define PWRES_CODE_REQUEST_RANGE_FAILED		pw::ResultCode::REQUEST_RANGE_FAILED
#define PWRES_CODE_EXPECT_FAILED			pw::ResultCode::EXPECT_FAILED
#define PWRES_CODE_INTERNAL_SERVER_ERROR	pw::ResultCode::INTERNAL_SERVER_ERROR
#define PWRES_CODE_NOT_IMPLEMENTED			pw::ResultCode::NOT_IMPLEMENTED
#define PWRES_CODE_BAD_GATEWAY				pw::ResultCode::BAD_GATEWAY
#define PWRES_CODE_SERVICE_UNAVAILABLE		pw::ResultCode::SERVICE_UNAVAILABLE
#define PWRES_CODE_GATEWAY_TIMEOUT			pw::ResultCode::GATEWAY_TIMEOUT
#define PWRES_CODE_VERSION_NOT_SUPPORTED	pw::ResultCode::VERSION_NOT_SUPPORTED

inline ResultCode& operator << (ResultCode& out, int v) { return ( out = static_cast<ResultCode>(v)); }

//! \brief C-style 문자열을 ResultCode형식으로 변환
inline ResultCode& operator << (ResultCode& out, const char* v) { return (out = static_cast<ResultCode>(::atoi(v))); }

//! \brief C-style 문자열을 ResultCode형식으로 변환
inline ResultCode& operator << (ResultCode& out, const std::string& v) { return (out = static_cast<ResultCode>(::atoi(v.c_str()))); }

//! \brief ResultCode를 스트림으로 출력
inline std::ostream& operator << (std::ostream& os, const ResultCode& v) { return os << static_cast<int>(v); }

//! \brief ResultCode를 스트링 객체로 출력
inline std::string& operator << (std::string& os, const ResultCode& v)
{
	char tmp[3+1];
	size_t len(snprintf(tmp, sizeof(tmp), "%d", static_cast<int>(v)));
	os.assign(tmp, std::min(static_cast<size_t>(sizeof(tmp)-1), len));
	return os;
}

extern ResultCode checkResultCode(ResultCode code);
inline ResultCode checkResultCode(int code) { return checkResultCode(static_cast<ResultCode>(code)); }
extern const char* getErrorMessageA(ResultCode code);
extern const std::string& getErrorMessage(ResultCode code);
extern std::string& getErrorMessage(std::string& out, ResultCode code);

#if (SIZEOF_SIZE_T == 4)
#	define strtossize(x,y,z)	ssize_t(strtol(x,y,z))
#	define strtosize(x,y,z)		size_t(strtoul(x,y,z))
#elif (SIZEOF_SIZE_T == 8)
#	define strtossize(x,y,z)	ssize_t(strtoll(x,y,z))
#	define strtosize(x,y,z)		size_t(strtoull(x,y,z))
#else
#	error "WHAT SIZE OF size_t TYPE!?"
#endif

constexpr size_t PW_CODE_SIZE = 4;

template<typename _Char>
struct char_ci_less
{
#if __cplusplus >= 201402L
	inline constexpr bool operator () (const _Char c1, const _Char c2) const
	{
		const auto& f = std::use_facet<std::ctype<_Char>>(std::locale());
		return f.toupper(c1) < f.toupper(c2);
	}
#else
	inline bool operator () (const _Char c1, const _Char c2) const
	{
		const auto& f = std::use_facet<std::ctype<_Char>>(std::locale());
		return f.toupper(c1) < f.toupper(c2);
	}
#endif
};

template<typename _Char>
inline
bool
char_ci_less_func(const _Char c1, const _Char c2)
{
	static const auto& f = std::use_facet<std::ctype<_Char>>(std::locale());
	return f.toupper(c1) < f.toupper(c2);
}

//! \brief 대소문자 가리지 않는 문자열 비교
template<typename _Str>
struct str_ci_less
{
	bool operator () (const _Str& s1, const _Str& s2) const
	{
		return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), char_ci_less_func<typename _Str::value_type>);
	}
};

struct host_type;
class uri_type;
struct url_type;

//! \brief host_type 리스트 타입
using host_list_type = std::list<host_type>;

//! \brief HOST:SERVICE 타입
struct host_type
{
	std::string	host;	//!< 호스트
	std::string	service;//!< 서비스

	inline host_type() {}
	inline host_type(const char* _host, const char* _service) : host(_host), service(_service) {}
	inline host_type(const std::string& line) { read(line); }
	host_type(const uri_type& uri);
	inline virtual ~host_type() {}

	bool read(const std::string& line);
	std::string& write(std::string& line) const;

	inline void swap(host_type& v)
	{
		host.swap(v.host);
		service.swap(v.service);
	}

	inline void clear(void)
	{
		host.clear();
		service.clear();
	}

	inline std::string str(void) const { std::string tmp; return this->write(tmp); }

	static host_list_type& s_read(host_list_type& out, const std::string& line);
	static std::string& s_write(std::string& line, const host_list_type& in);

	inline host_type& operator = (const std::string& line) { read(line); return *this; }
	inline host_type& operator = (const host_type&) = default;
	inline host_type& operator = (const url_type& url);
	host_type& operator = (const uri_type& uri);

	inline std::ostream& operator << (std::ostream& os) { std::string tmp; os << write(tmp); return os; }

	inline host_type& operator () (const std::string& _host, const std::string& _service)
	{
		this->host = _host;
		this->service = _service;
		return *this;
	}

	inline bool operator < (const host_type& v) const
	{
		if ( host == v.host )
		{
			return service < v.service;
		}

		return host < v.host;
	}

	inline bool operator == (const host_type& v) const
	{
		if ( this == &v ) return true;
		if (host == v.host) return (service == v.service);
		return false;
	}

	inline bool operator not_eq (const host_type& v) const
	{
		if ( this == &v ) return false;
		if ( host == v.host ) return (service not_eq v.service);
		return true;
	}
};

//! \brief url_type 리스트 타입
using url_list_type = std::list<url_type>;

//! \brief HOST:SERVICE/PAGE 타입
struct url_type
{
	std::string		host;		//!< 호스트
	std::string		service;	//!< 서비스
	std::string		page;		//!< 페이지

	inline url_type() {}
	inline url_type(const char* _host, const char* _service, const char* _page) : host(_host), service(_service), page(_page) {}
	inline url_type(const std::string& line) { read(line); }
	url_type(const uri_type& uri);
	inline virtual ~url_type() {}

	bool read(const std::string& line);
	std::string& write(std::string& line) const;

	inline void swap(url_type& v)
	{
		host.swap(v.host);
		service.swap(v.service);
		page.swap(v.page);
	}

	inline void clear(void)
	{
		host.clear();
		service.clear();
		page.clear();
	}

	inline std::string str(void) const { std::string tmp; return this->write(tmp); }

	static url_list_type& s_read(url_list_type& out, const std::string& line);
	static std::string& s_write(std::string& line, const url_list_type& in);

	inline url_type& operator = (const std::string& line) { read(line); return *this; }
	inline url_type& operator = (const host_type& host);
	url_type& operator = (const uri_type& uri);
	inline std::ostream& operator << (std::ostream& os) { std::string tmp; os << write(tmp); return os; }
	inline operator host_type (void) const { return host_type(host.c_str(), service.c_str()); }
	inline url_type& operator () (const std::string& _host, const std::string& _service, const std::string& _page)
	{
		this->host = _host;
		this->service = _service;
		this->page = _page;
		return *this;
	}

	inline bool operator < (const url_type& v) const
	{
		if ( host == v.host )
		{
			if ( service == v.service )
			{
				return page < v.page;
			}

			return service < v.service;
		}

		return host < v.host;
	}

	inline bool operator == (const url_type& v) const
	{
		if ( this == &v ) return true;
		if (host == v.host)
		{
			if (service == v.service) return (page == v.page);
		}
		return false;
	}

	inline bool operator not_eq (const url_type& v) const
	{
		if ( this == &v ) return false;
		if ( host == v.host )
		{
			if (service == v.service) return (page not_eq v.page);
		}
		return true;
	}
};

//! \brief BLOB 타입
struct blob_type final
{
	//! \brief 복제 형태
	typedef enum copy_type
	{
		CT_MALLOC,	//!< 실제 복제 되어 있음
		CT_POINTER,//!< 포인터만 복제 되어 있음
	}copy_type;

	copy_type type {CT_MALLOC};
	const char* buf {nullptr};
	size_t size {0};

	inline blob_type() = default;
	inline blob_type(const void* _buf, size_t _size, copy_type _type) : type(CT_MALLOC), buf(nullptr), size(0) { assign(_buf, _size, _type); }
	inline explicit blob_type(const blob_type& blob, copy_type _type = CT_MALLOC) : blob_type(blob.buf, blob.size, _type) {}
	inline explicit blob_type(const std::string& s, copy_type _type) : blob_type(s.c_str(), s.size(), _type) {}
	inline explicit blob_type(const char* cstr, copy_type _type) : blob_type(cstr, ::strlen(cstr), _type) {}

	inline explicit blob_type(blob_type&& blob) : type(blob.type), buf(blob.buf), size(blob.size)
	{
		blob.buf = nullptr;
		blob.size = 0;
	}

	inline virtual ~blob_type()
	{
		if ( buf )
		{
			if ( CT_MALLOC == type )
			{
				::free(const_cast<char*>(buf));
			}

			buf = nullptr;
		}
	}

	inline void clear(void)
	{
		if ( buf )
		{
			if ( CT_MALLOC == type )
			{
				::free(const_cast<char*>(buf));
			}

			buf = nullptr;
		}

		type = CT_MALLOC;
		size = 0;
	}

	inline bool empty(void) const
	{
		return ( (nullptr == buf) or (0 == size) );
	}

	inline void swap(blob_type& v)
	{
		std::swap(type, v.type);
		std::swap(buf, v.buf);
		std::swap(size, v.size);
	}

	inline const char* begin(void) const { return buf; }
	inline const char* cbegin(void) const { return buf; }
	inline const char* end(void) const { return ( buf ? buf + size : nullptr ); }
	inline const char* cend(void) const { return ( buf ? buf + size : nullptr ); }

	inline bool allocate(size_t len)
	{
		if ( 0 == len ) return false;

		const char* nbuf(static_cast<const char*>(::malloc(len)));
		if ( nullptr == nbuf ) return false;

		blob_type tmp(nbuf, len, CT_POINTER);
		swap(tmp);

		type = CT_MALLOC;

		return true;
	}

	inline bool assign(const blob_type& b, copy_type ct)
	{
		blob_type tmp;
		if ( ct == CT_MALLOC )
		{
			if ( not b.empty() )
			{
				void* buf(::malloc(b.size));
				if ( nullptr == buf )
				{
					return false;
				}

				::memcpy(buf, b.buf, b.size);
				tmp.buf = (char*)buf;
				tmp.size = b.size;
				tmp.type = CT_MALLOC;
			}
		}
		else
		{
			tmp.type = CT_POINTER;
			tmp.buf = b.buf;
			tmp.size = b.size;
		}

		swap(tmp);
		return true;
	}

	inline bool assign(const void* ibuf, size_t iblen, copy_type ct)
	{
		blob_type tmp;
		if ( ct == CT_MALLOC )
		{
			if ( nullptr not_eq ibuf )
			{
				void* buf(::malloc(iblen));
				if ( nullptr == buf )
				{
					return false;
				}

				::memcpy(buf, ibuf, iblen);
				tmp.buf = (char*)buf;
				tmp.size = iblen;
				tmp.type = CT_MALLOC;
			}
		}
		else
		{
			tmp.type = CT_POINTER;
			tmp.buf = (char*)ibuf;
			tmp.size = iblen;
		}

		swap(tmp);
		return true;
	}

	inline bool assign(const std::string& s, copy_type ct)
	{
		return this->assign(s.c_str(), s.size(), ct);
	}

	bool format(const char* fmt, ...) __attribute__((format(printf,2,3)));

	bool append(size_t pos, const void* buf, size_t blen);
	inline bool append(const void* buf, size_t blen) { return append(size, buf, blen); }

	inline bool isNull(void) const { return ( (not buf) and (not size) ); }

public:
	inline blob_type& operator = (const blob_type& b) { assign(b, CT_MALLOC); return *this; }
	inline blob_type& operator = (blob_type&& b) { buf = b.buf; size = b.size; type = b.type; b.buf = nullptr; b.size = 0; return *this; }
	inline blob_type& operator = (const char* str) { assign(str, strlen(str), CT_MALLOC); return *this; }
	inline blob_type& operator = (const std::string& s) { assign(s.c_str(), s.size(), CT_MALLOC); return *this; }

	inline char& operator[] (size_t index) { return const_cast<char*>(buf)[index]; }
	inline const char& operator[] (size_t index) const { return buf[index]; }

	inline bool operator == (const blob_type& b) const
	{
		if ( this == &b ) return true;
		if ( buf == b.buf ) return size == b.size;
		if ( size not_eq b.size ) return false;

		return ::memcmp(buf, b.buf, size) == 0;
	}
};

inline
std::string&
operator << (std::string& s, const blob_type& b)
{
	if ( not b.empty() ) s.append(b.buf, b.size);
	return s;
}

inline
std::ostream&
operator << (std::ostream& os, const blob_type& b)
{
	if ( not b.empty() ) os.write(b.buf, b.size);
	return os;
}

template<typename _Type>
inline
_Type
swapEndian(const _Type& v)
{
	constexpr auto size(sizeof(_Type));
	union {
		_Type x;
		uint8_t u8[size];
	} k{v};
	auto* p(&(k.u8[0]));
	std::reverse(p, p+size);
	return k.x;
}

template<typename _Type>
inline
_Type&
convertEndian(_Type& v)
{
	constexpr auto size(sizeof(_Type));
	union inner_type {
		_Type x;
		uint8_t u8[size];
	}* k(reinterpret_cast<inner_type*>(&v));
	auto* p(&(k->u8[0]));
	std::reverse(p, p+size);
	return v;
}

template<typename _Type>
inline
_Type
swapNetworkEndian(const _Type& v)
{
#ifdef WORDS_BIGENDIAN
	return v;
#else
	return swapEndian<_Type>(v);
#endif
}

template<typename _Type>
inline
_Type&
convertNetworkEndian(_Type& v)
{
#ifdef WORDS_BIGENDIAN
	return v;
#else
	return convertEndian(v);
#endif
}

#ifdef WORDS_BIGENDIAN
#	define PW_SWAP_NET_ENDIAN(x) (x)
#	define PW_CONV_NET_ENDIAN(x) (x)
#else
#	define PW_SWAP_NET_ENDIAN(x) swapEndian(x)
#	define PW_CONV_NET_ENDIAN(x) convertEndian(x)
#endif

//! \brief 많이 쓰이는 Key Value 쌍
using keyvalue_cont = std::map<std::string, std::string>;

//! \brief 많이 쓰이는 Key Value 쌍. 멀티맵
using keyvalue2_cont = std::multimap<std::string, std::string>;

//! \brief 많이 쓰이는 Key Value 쌍. 대소문자 가리지 않는다.
using keyivalue_cont = std::map<std::string, std::string, str_ci_less<std::string>>;

//! \brief 많이 쓰이는 Key Value 쌍. 멀티맵. 대소문자 가리지 않는다.
using keyivalue2_cont = std::multimap<std::string, std::string, str_ci_less<std::string>>;

//! \brief 많이 쓰이는 std::list<std::string>
using string_list = std::list<std::string>;

//! \brief 많이 쓰이는 std::set<std::string>;
using string_set = std::set<std::string>;

//! \brief 많이 쓰이는 std::unordered_set<std::string>;
using string_uset = std::unordered_set<std::string>;

//! \brief 많이 쓰이는 std::map<std::string, void*>
using strptr_cont = std::map<std::string, void*>;

template<typename _Type>
using strobj_cont = std::map<std::string, _Type>;

//! \brief 많이 쓰이는 std::map<std::string, void*>
using striptr_cont = std::map<std::string, void*, str_ci_less<std::string>>;

template<typename _Type>
using striobj_cont = std::map<std::string, _Type, str_ci_less<std::string>>;

//! \brief 많이 쓰이는 std::map<int, void*>
using intptr_cont = std::map<int, void*>;

//! \brief 많이 쓰이는 std::map<std::string. int>
using strint_cont = std::map<std::string, int>;
using striint_cont = std::map<std::string, int, str_ci_less<std::string>>;
using intstr_cont = std::map<int, std::string>;
using intint_cont = std::map<int, int>;

host_type&
host_type::operator= ( const url_type& url )
{
	this->operator()(url.host, url.service);
	return *this;
}

url_type&
url_type::operator= ( const host_type& host )
{
	this->operator()(host.host, host.service, std::string());
	return *this;
}

//! \brief 기본 삭제자
template<typename _Type>
struct deleter {
    inline void operator() (_Type* ptr) { if (ptr) _Type::s_release(ptr); }
};

}; //namespace pw

#endif//!__PW_COMMON_H__

