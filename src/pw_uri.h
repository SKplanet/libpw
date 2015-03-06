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
 * \file pw_uri.h
 * \brief Support URI(RFC 3986) with URIParser(http://uriparser.sourceforge.net/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_URI_H__
#define __PW_URI_H__

namespace pw {

//! \brief UriParser를 이용한 URI 클래스
class uri_type final
{
public:
	using string_pointer = std::string*;
	using path_list = std::list<string_pointer>;

public:
	explicit uri_type(const uri_type& obj);
	explicit uri_type(uri_type&& obj);
	inline explicit uri_type(const char* s) { parse(s); }
	inline explicit uri_type(const std::string& s) { parse(s); }
	inline ~uri_type() = default;
	inline explicit uri_type() = default;

	uri_type& operator = (const uri_type& obj);
	uri_type& operator = (uri_type&& obj);
	inline uri_type& operator = (const char* s) { parse(s); return *this; }
	inline uri_type& operator = (const std::string& s) { parse(s); return *this; }

public:
	//! \brief URI를 파싱한다.
	bool parse(const char* s);
	bool parse(const char* s, size_t slen);
	inline bool parse(const std::string& s) { return this->parse(s.c_str(), s.size()); }

	//! \brief 절대경로를 만든다.
	//! \param[out] out 최종 경로.
	//! \param[in] base 절대 경로의 베이스.
	bool addBase(uri_type& out, const uri_type& base) const;
	bool addBase(const uri_type& base);

	//! \brief 상대경로를 만든다.
	//! \param[out] out 최종 경로.
	//! \param[in] base 절대 경로의 베이스.
	bool removeBase(uri_type& out, const uri_type& base, bool useDomainRoot = false) const;
	bool removeBase(const uri_type& base, bool useDomainRoot = false);

public:
	//! \brief 스왑한다.
	inline void swap(uri_type& in) { data.swap(in.data); }

	//! \brief 내용을 지운다.
	inline void clear(void) { data.clear(); }

	//! \brief 문자열로 재조합한다.
	std::string& str(std::string& out) const;
	inline std::string str(void) const { std::string tmp; return this->str(tmp); }

	//! \brief 포트를 숫자로 반환한다.
	int getNumericPort(void) const;

	//! \brief 서비스로 포트를 설정한다.
	bool setPortByService(const std::string& in);

	//! \brief 스킴으로 포트를 설정한다.
	bool setPortByScheme(void);

	//! \brief 패스를 문자열로 반환한다.
	std::string& getPathString(std::string& out) const;
	std::string getPathString(void) const;

	//! \brief 패스를 설정한다.
	void setPath(const std::string& in);

	//! \brief 쿼리 부분을 반환한다.
	keyvalue_cont& getQuery(keyvalue_cont& out) const;

	//! \brief URI에 불필요한 부분을 제거한다.
	bool normalize(void);
	bool normalize(uri_type& out) const;

	inline std::string getScheme(void) const { return _s_getString(data.scheme); }
	inline void setScheme(const std::string& in) { _s_setString(data.scheme, in); }
	inline bool isNullScheme(void) const { return nullptr == data.scheme; }
	inline void setNullScheme(void) { if ( data.scheme ) { delete data.scheme; data.scheme = nullptr; } }
	inline std::string& getRefOfScheme(void) { return *data.scheme; }
	inline const std::string& getRefOfScheme(void) const { return *data.scheme; }

	inline std::string getUserInfo(void) const { return _s_getString(data.userInfo); }
	inline void setUserInfo(const std::string& in) { _s_setString(data.userInfo, in); }
	inline bool isNullUserInfo(void) const { return nullptr == data.userInfo; }
	inline void setNullUserInfo(void) { if ( data.userInfo ) { delete data.userInfo; data.userInfo = nullptr; } }
	inline std::string& getRefOfUserInfo(void) { return *data.userInfo; }
	inline const std::string& getRefOfUserInfo(void) const { return *data.userInfo; }

	inline std::string getHost(void) const { return _s_getString(data.host); }
	inline void setHost(const std::string& in) { _s_setString(data.host, in); }
	inline bool isNullHost(void) const { return nullptr == data.host; }
	inline void setNullHost(void) { if ( data.host ) { delete data.host; data.host = nullptr; } }
	inline std::string& getRefOfHost(void) { return *data.host; }
	inline const std::string& getRefOfHost(void) const { return *data.host; }

	inline std::string getPort(void) const { return _s_getString(data.port); }
	inline void setPort(const std::string& in) { _s_setString(data.port, in); }
	inline bool isNullPort(void) const { return nullptr == data.port; }
	inline void setNullPort(void) { if ( data.port ) { delete data.port; data.port = nullptr; } }
	inline std::string& getRefOfPort(void) { return *data.port; }
	inline const std::string& getRefOfPort(void) const { return *data.port; }

	inline std::string getQuery(void) const { return _s_getString(data.query); }
	inline void setQuery(const std::string& in) { _s_setString(data.query, in); }
	inline bool isNullQuery(void) const { return nullptr == data.query; }
	inline void setNullQuery(void) { if ( data.query ) { delete data.query; data.query = nullptr; } }
	inline std::string& getRefOfQuery(void) { return *data.query; }
	inline const std::string& getRefOfQuery(void) const { return *data.query; }

	inline std::string getFragment(void) const { return _s_getString(data.fragment); }
	inline void setFragment(const std::string& in) { _s_setString(data.fragment, in); }
	inline bool isNullFragment(void) const { return nullptr == data.fragment; }
	inline void setNullFragment(void) { if ( data.fragment ) { delete data.fragment; data.fragment = nullptr; } }
	inline std::string& getRefOfFragment(void) { return *data.fragment; }
	inline const std::string& getRefOfFragment(void) const { return *data.fragment; }

	inline bool isEmptyPath(void) const { return data.path.empty(); }
	inline void appendToPath(const std::string& in) { string_pointer p(nullptr); _s_setString(p, in); data.path.push_back(p); }
	inline void appendNullToPath(void) { data.path.push_back(nullptr); }
	inline void clearPath(void) { _s_release(data.path); }
	inline path_list& getRefOfPath(void) { return data.path; }
	inline const path_list& getRefOfPath(void) const { return data.path; }

	inline bool isAbsolute(void) const { return data.isAbs; }
	inline void setAbsolute(bool a) { data.isAbs = a; }

private:
	inline static std::string _s_getString(const std::string* in) { return in ? *in : std::string(); }
	inline static void _s_setString(string_pointer& out, const std::string& in)
	{
		if ( out ) { *out = in; return; }
		if ( nullptr == (out = new std::string(in)) ) throw std::bad_alloc();
	}

	inline static void _s_release(path_list& out) { path_list tmp; tmp.swap(out); for ( auto& p : tmp ) { if ( p ) delete p; } }
	inline static void _s_release(string_pointer& out) { if ( out ) { delete out; out = nullptr; } }

public:
	struct inner_type final {
		std::string* scheme = nullptr;
		std::string* userInfo = nullptr;
		std::string* host = nullptr;
		std::string* port = nullptr;
		std::string* query = nullptr;
		std::string* fragment = nullptr;

		path_list path;
		bool isAbs = false;

		inline void swap(inner_type& obj)
		{
			std::swap(scheme, obj.scheme);
			std::swap(userInfo, obj.userInfo);
			std::swap(host, obj.host);
			std::swap(port, obj.port);
			path.swap(obj.path);
			std::swap(query, obj.query);
			std::swap(fragment, obj.fragment);
			std::swap(isAbs, obj.isAbs);
		}

		void clear(void);

		~inner_type();
	};

private:
	inner_type data;

private:
	void _setToData(void* out) const;
	void _setFromData(const void* in);
};

//namespace pw
};

extern std::ostream& operator << (ostream& out, const pw::uri_type& uri);

#endif//__PW_URI_H__
