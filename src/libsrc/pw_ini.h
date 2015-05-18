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
 * \file pw_ini.h
 * \brief Support INI file parser.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_uri.h"

#ifndef __PW_INI_H__
#define __PW_INI_H__

namespace pw
{

//! @brief INI 파서
class Ini
{
public:
	//! @brief SAX 방식으로 해석하기 위한 이벤트 처리
	class Event
	{
	protected:
		inline virtual bool eventSection(const std::string& v) { return true; }
		inline virtual bool eventItem(const std::string& value, const std::string& itemname, const std::string& secname, bool& add) { return true; }

	protected:
		inline virtual ~Event() {}

	friend class Ini;
	};


public:
	using item_cont = std::map<std::string, std::string>; //!< 아이템 컨테이너
	using item_citr = item_cont::const_iterator;
	using item_itr = item_cont::iterator;

	using sec_cont = std::map<std::string, item_cont>; //!< 섹션 컨테이너
	using sec_citr = sec_cont::const_iterator;
	using sec_itr = sec_cont::iterator;

public:
	explicit Ini(const char* path, Event* e = nullptr);
	explicit Ini(std::istream& is, Event* e = nullptr);
	explicit Ini();
	inline virtual ~Ini() = default;

public:
	bool read(const char* path, Event* e = nullptr);
	bool read(std::istream& is, Event* e = nullptr);
	bool write(const char* path, Event* e = nullptr) const;
	bool write(std::ostream& os, Event* e = nullptr) const;
	inline void swap(Ini& v) { m_cont.swap(v.m_cont); }

public:
	bool getBoolean(const std::string& item, const std::string& sec, bool def) const;
	intmax_t getInteger(const std::string& item, const std::string& sec, intmax_t def) const;
	long double getReal(const std::string& item, const std::string& sec, long double def) const;
	std::string getString(const std::string& item, const std::string& sec, const std::string& def) const;
	std::string& getString2(std::string& out, const std::string& item, const std::string& sec, const std::string& def = std::string()) const;
	bool getHost(host_type& host, const std::string& item, const std::string& sec) const;
	host_list_type& getHostList(host_list_type& host, const std::string& item, const std::string& sec) const;
	bool getUrl(url_type& url, const std::string& item, const std::string& sec) const;
	url_list_type& getUrlList(url_list_type& url, const std::string& item, const std::string& sec) const;
	bool getUri(uri_type& uri, const std::string& item, const std::string& sec) const;

	bool getBoolean(const std::string& item, sec_citr sec, bool def) const;
	intmax_t getInteger(const std::string& item, sec_citr sec, intmax_t def) const;
	long double getReal(const std::string& item, sec_citr sec, long double def) const;
	std::string getString(const std::string& item, sec_citr sec, const std::string& def) const;
	std::string& getString2(std::string& out, const std::string& item, sec_citr sec, const std::string& def = std::string()) const;
	bool getHost(host_type& host, const std::string& item, sec_citr sec) const;
	host_list_type& getHostList(host_list_type& host, const std::string& item, sec_citr sec) const;
	bool getUrl(url_type& url, const std::string& item, sec_citr sec) const;
	url_list_type& getUrlList(url_list_type& url, const std::string& item, sec_citr sec) const;
	bool getUri(uri_type& uri, const std::string& item, sec_citr sec) const;

public:
	void setBoolean(bool v, const std::string& item, const std::string& sec);
	void setBoolean(bool v, const std::string& item, sec_itr sec);

	void setInteger(intmax_t v, const std::string& item, const std::string& sec);
	void setInteger(intmax_t v, const std::string& item, sec_itr sec);

	void setReal(long double v, const std::string& item, const std::string& sec);
	void setReal(long double v, const std::string& item, sec_itr sec);

	void setString(const std::string& v, const std::string& item, const std::string& sec);
	void setString(const std::string& v, const std::string& item, sec_itr sec);

	void setHost(const host_type& v, const std::string& item, const std::string& sec);
	void setHost(const host_type& v, const std::string& item, sec_itr sec);

	void setHostList(const host_list_type& v, const std::string& item, const std::string& sec);
	void setHostList(const host_list_type& v, const std::string& item, sec_itr sec);

	void setUrl(const url_type& v, const std::string& item, const std::string& sec);
	void setUrl(const url_type& v, const std::string& item, sec_itr sec);

	void setUrlList(const url_list_type& v, const std::string& item, const std::string& sec);
	void setUrlList(const url_list_type& v, const std::string& item, sec_itr sec);

	void setUri(const uri_type& v, const std::string& item, const std::string& sec);
	void setUri(const uri_type& v, const std::string& item, sec_itr sec);

public:
	inline sec_citr cbegin(void) const { return m_cont.begin(); }
	inline sec_citr begin(void) const { return m_cont.begin(); }
	inline sec_itr begin(void) { return m_cont.begin(); }
	inline sec_citr cend(void) const { return m_cont.end(); }
	inline sec_citr end(void) const { return m_cont.end(); }
	inline sec_itr end(void) { return m_cont.end(); }

	inline sec_citr find(const std::string& secname) const { return m_cont.find(secname); }
	inline sec_citr cfind(const std::string& secname) const { return m_cont.find(secname); }
	inline sec_itr find(const std::string& secname) { return m_cont.find(secname); }

protected:
	inline std::string* getItem(const std::string& item, const std::string& sec)
	{
		sec_itr ib(m_cont.find(sec));
		if ( ib == m_cont.end() ) return nullptr;

		item_itr it(ib->second.find(item));
		if ( it == ib->second.end() ) return nullptr;

		return &(it->second);
	}

	inline const std::string* getItem(const std::string& item, const std::string& sec) const
	{
		sec_citr ib(m_cont.find(sec));
		if ( ib == m_cont.end() ) return nullptr;

		item_citr it(ib->second.find(item));
		if ( it == ib->second.end() ) return nullptr;

		return &(it->second);
	}

protected:
	sec_cont		m_cont;
};

inline
std::ostream&
operator << (std::ostream& os, const Ini& ini)
{
	ini.write(os);
	return os;
}

}; //namespace pw

#endif//!__PW_INI_H__
