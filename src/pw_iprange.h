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
 * \file pw_iprange.h
 * \brief Support IP range for IPv4 and IPv6.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_IPRANGE_H__
#define __PW_IPRANGE_H__

#include "./pw_iprange_type.h"

namespace pw {

//! \brief 기존 버전과 달리 모든 대역을 다룬다.
class IpRange final
{
public:
	enum class IpVersion : int
	{
		V4 = 4,
		V6 = 6,
	};

	enum class FileType
	{
		TEXT,
		JSON,
		SQLITE,
	};

	static const std::string s_ipv4_str;
	static const std::string s_ipv6_str;

public:
	inline static const std::string&	s_toString (IpVersion v)
	{
		return (v == IpVersion::V4 ? s_ipv4_str : s_ipv6_str);
	}

	static IpVersion s_toIpVersion (const char* s);
	static IpVersion s_toIpVersion (const std::string& s);

public:
	inline	IpRange () = default;
	inline	IpRange (const IpRange&) = default;
	inline	IpRange (IpRange&&) = default;
	inline IpRange&	operator = (const IpRange&) = default;
	inline IpRange&	operator = (IpRange&&) = default;

public:
	inline void clear (void)
	{
		m_cont4.clear();
#ifdef PW_HAVE_INT128
		m_cont6.clear();
#endif
	}

	inline void swap (IpRange& v)
	{
		if (this == &v) return;
		m_cont4.swap(v.m_cont4);
#ifdef PW_HAVE_INT128
		m_cont6.swap(v.m_cont6);
#endif
	}

	inline bool read(const char* fn, FileType ft = FileType::TEXT, const char* param = nullptr)
	{
		switch(ft)
		{
		case FileType::TEXT: return readAsText(fn);
		case FileType::JSON: return readAsJson(fn, param);
		case FileType::SQLITE: return readAsSqlite(fn, param);
		}

		return false;
	}
	inline bool read(std::istream& is, FileType ft = FileType::TEXT, const char* param = nullptr)
	{
		switch(ft)
		{
			case FileType::TEXT: return readAsText(is);
			case FileType::JSON: return readAsJson(is, param);
			case FileType::SQLITE: return readAsSqlite(is, param);
		}

		return false;
	}
	bool readAsText(const char* fn);
	bool readAsText(std::istream& is);
	bool readAsJson(const char* fn, const char* rootname);
	bool readAsJson(std::istream& is, const char* rootname);
	bool readAsSqlite(const char* fn, const char* tablename);
	bool readAsSqlite(std::istream& is, const char* tablename) { return false; }

	inline bool write(const char* fn, FileType ft = FileType::TEXT, const char* param = nullptr) const
	{
		switch(ft)
		{
			case FileType::TEXT: return writeAsText(fn);
			case FileType::JSON: return writeAsJson(fn, param);
			case FileType::SQLITE: return writeAsSqlite(fn, param);
		}

		return false;
	}
	inline bool write(std::ostream& os, FileType ft = FileType::TEXT, const char* param = nullptr) const
	{
		switch(ft)
		{
			case FileType::TEXT: return writeAsText(os);
			case FileType::JSON: return writeAsJson(os, param);
			case FileType::SQLITE: return writeAsSqlite(os, param);
		}

		return false;
	}
	bool writeAsText(const char* fn) const;
	bool writeAsText(std::ostream& os) const;
	bool writeAsJson(const char* fn, const char* rootname) const;
	bool writeAsJson(std::ostream& os, const char* rootname) const;
	bool writeAsSqlite(const char* fn, const char* tablename) const;
	bool writeAsSqlite(std::ostream& os, const char* tablename) const { return false; }
	bool dump(std::ostream& os) const;

	bool insert(IpVersion ver, const std::string& start, const std::string& end, const std::string& value);
	bool insertByCIDR(IpVersion ver, const std::string& base, size_t cidr_bit_count, const std::string& value);

	inline bool find(const char* ip) const
	{
		if ( find4(ip) ) return true;
		return find6(ip);
	}

	inline bool find(std::string& out, const char* ip) const
	{
		if ( find4(out, ip) ) return true;
		return find6(out, ip);
	}

	bool find4(const char* ip) const;
	bool find4(std::string& out, const char* ip) const;
	bool find6(const char* ip) const;
	bool find6(std::string& out, const char* ip) const;

	template<typename _Type>
	inline bool find(const _Type& ip) const { return this->find(ip.c_str()); }

	template<typename _Type>
	inline bool find(std::string& out, const _Type& ip) const { return this->find(out, ip.c_str()); }

	template<typename _Type>
	inline bool find4(const _Type& ip) const { return this->find4(ip.c_str()); }

	template<typename _Type>
	inline bool find4(std::string& out, const _Type& ip) const { return this->find4(out, ip.c_str()); }

	template<typename _Type>
	inline bool find6(const _Type& ip) const { return this->find6(ip.c_str()); }

	template<typename _Type>
	inline bool find6(std::string& out, const _Type& ip) const { return this->find6(out, ip.c_str()); }

private:
	using v4_cont = std::map<iprange::ItemIpV4, std::string>;
	v4_cont m_cont4;

#ifdef PW_HAVE_INT128
	using v6_cont = std::map<iprange::ItemIpV6, std::string>;
	v6_cont m_cont6;
#endif

private:
	static int _cb_read(void*, int, char**, char**);
	bool _read_json(const void* json);

};//IpRange

};//namespace pw
#endif// __PW_IPRANGE_H__
