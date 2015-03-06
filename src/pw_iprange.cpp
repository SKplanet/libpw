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
 * \file pw_iprange.cpp
 * \brief Support IP range for IPv4 and IPv6.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "pw_string.h"
#include "pw_iprange.h"
#include "pw_socket.h"
#include "./pw_log.h"
#include <bitset>
#include <arpa/inet.h>

#if HAVE_SQLITE3
#	include <sqlite3.h>
#endif

#if HAVE_JSONCPP
#	include <json/json.h>
#endif

#define DEF_TABLE_NAME "ip_range"

namespace pw {

const std::string IpRange::s_ipv4_str("ipv4");
const std::string IpRange::s_ipv6_str("ipv6");

template<typename _Type>
struct insert_type {
	using value_type = _Type;

	value_type begin;
	value_type end;
	std::string value;
	std::string line;
	size_t line_count = 0;
};

template<typename _Type>
_Type
__makeMaskByCIDR(size_t bc)
{
}

template<>
uint32_t
__makeMaskByCIDR<uint32_t>(size_t bc)
{
	std::bitset<sizeof(uint32_t)> bs;
	for ( size_t i (0); i < bc; i++ ) bs[i] = true;
	return bs.to_ulong();
}

template<>
uint128_t
__makeMaskByCIDR<uint128_t>(size_t bc)
{
	std::bitset<64> high, low;
	if ( bc < 64 )
	{
		for ( size_t i(0); i < bc; i++ ) high[i] = true;
	}
	else
	{
		high.flip();
		for ( size_t i(0); i < 64-bc; i++ ) low[i] = true;
	}

	union {
		uint64_t u64[2];
		uint128_t u128;
	} u { high.to_ullong(), low.to_ullong() };

	return u.u128;
}

template<typename _InsType>
bool
__setInsertType(_InsType& out, const std::string& base, size_t cidr_bit_count)
{
	using value_type = typename _InsType::value_type;
	using item_type = iprange::ItemTemplate<value_type>;

	value_type base_int;
	if ( not item_type::s_toValue(base_int, base) ) return false;

	value_type mask(PW_SWAP_NET_ENDIAN(__makeMaskByCIDR<value_type>(cidr_bit_count)));

	out.begin = (base_int bitand mask);
	out.end = (out.begin bitor (compl mask));

	return true;
}

template<typename _Type, typename _InsType>
static
bool
__insert(_Type& type, const _InsType& ins)
{
	using value_type = typename _Type::value_type;
	using key_type = typename _Type::key_type;

	const auto res(type.insert(value_type(key_type(ins.begin, ins.end), ins.value)));

	if (res.second) return true;

	if ( res.first == type.end() )
	{
		PWLOGLIB("failed to insert. not enough memory or corrupt map: line:%zu buf:%s", ins.line_count, ins.line.c_str());
	}
	else
	{
		const auto& key(res.first->first);
		const auto& value(res.first->second);
		PWLOGLIB("failed to insert. duplicated or crashed item: line:%zu pre-ins:%s %s buf:%s", ins.line_count, key.str().c_str(), value.c_str(), ins.line.c_str());
	}

	return false;
}

IpRange::IpVersion
IpRange::s_toIpVersion (const char* s)
{
	return (strcasecmp(s, s_ipv6_str.c_str()) == 0) ? IpVersion::V6 : IpVersion::V4;
}

IpRange::IpVersion
IpRange::s_toIpVersion (const std::string& s)
{
	return (strcasecmp(s.c_str(), s_ipv6_str.c_str()) == 0) ? IpVersion::V6 : IpVersion::V4;
}

bool
IpRange::find4(const char* ip) const
{
	auto ib(m_cont4.find( iprange::ItemIpV4(ip, ip)));
	return m_cont4.end() not_eq ib;
}

bool
IpRange::find4(std::string& out, const char* ip) const
{
	auto ib(m_cont4.find( iprange::ItemIpV4(ip, ip)));
	if ( m_cont4.end() not_eq ib )
	{
		out = ib->second;
		return true;
	}

	out.clear();
	return false;
}

bool
IpRange::find6(const char* ip) const
{
#ifdef PW_HAVE_INT128
	auto ib(m_cont6.find( iprange::ItemIpV6(ip, ip)));
	return m_cont6.end() not_eq ib;
#else
	return false;
#endif
}

bool
IpRange::find6(std::string& out, const char* ip) const
{
#ifdef PW_HAVE_INT128
	auto ib(m_cont6.find( iprange::ItemIpV6(ip, ip)));
	if ( m_cont6.end() not_eq ib )
	{
		out = ib->second;
		return true;
	}
#endif
	out.clear();
	return false;
}

bool
IpRange::insert ( pw::IpRange::IpVersion ver, const std::string& start, const std::string& end, const std::string& value )
{
	insert_type<std::string> ins;
	ins.begin = start;
	ins.end = end;
	ins.value = value;

	if ( ver == IpVersion::V4 ) return __insert(m_cont4, ins);
#ifdef PW_HAVE_INT128
	return __insert(m_cont6, ins);
#else
	return false;
#endif
}

bool
IpRange::insertByCIDR ( pw::IpRange::IpVersion ver, const std::string& base, size_t cidr_bit_count, const std::string& value )
{
	if ( ver == IpRange::IpVersion::V4 )
	{
		insert_type<uint32_t> ins;
		if ( not __setInsertType(ins, base, cidr_bit_count) )
		{
			PWLOGLIB("invalid base or bit count: %s", cstr(base));
			return false;
		}
		ins.value = value;
		return __insert(m_cont4, ins);
	}
#ifdef PW_HAVE_INT128
	else
	{
		insert_type<uint128_t> ins;
		if ( not __setInsertType(ins, base, cidr_bit_count) )
		{
			PWLOGLIB("invalid base or bit count: %s", cstr(base));
			return false;
		}
		ins.value = value;
		return __insert(m_cont6, ins);
	}
#endif

	return false;
}

bool
IpRange::readAsText (const char* fn)
{
	std::ifstream ifs(fn);
	if ( ifs.is_open() ) return this->readAsText(ifs);
	return false;
}

bool
IpRange::readAsJson (const char* fn, const char* rootname)
{
	//PWSHOWMETHOD();
	std::ifstream ifs(fn);
	if ( ifs.is_open() ) return this->readAsJson(ifs, rootname);
	return false;
}

bool
IpRange::readAsSqlite ( const char* fn, const char* tablename)
{
	//PWSHOWMETHOD();
#if HAVE_SQLITE3
	sqlite3* handle(nullptr);
	sqlite3_stmt* stmt(nullptr);
	char* err(nullptr);
	int res;
	v4_cont tmp4;
	#ifdef PW_HAVE_INT128
	v6_cont tmp6;
	#endif
	insert_type<std::string> ins;

	if ( not tablename )
	{
		tablename = DEF_TABLE_NAME;
	}

	do {
		if ( SQLITE_OK not_eq sqlite3_open(fn, &handle) )
		{
			PWLOGLIB("failed to open sqlite3 file: %s", fn);
			break;
		}

		const std::string sql(PWStr::formatS("SELECT type, ip_begin, ip_end, value FROM %s", tablename));
		const char* uncomp(nullptr);
		if ( SQLITE_OK not_eq sqlite3_prepare(handle, sql.c_str(), sql.size(), &stmt, &uncomp) )
		{
			PWLOGLIB("failed to prepare statment: %s", uncomp);
			break;
		}

		const unsigned char* from;
		const unsigned char* to;
		const unsigned char* value;

		while ( SQLITE_ROW == (res = sqlite3_step(stmt)) )
		{
			//PWTRACE("row!");
			++ ins.line_count;
			if ( nullptr == (from = sqlite3_column_text(stmt, 1)) )
			{
				PWLOGLIB("failed to get <from>");
				goto PROC_ERROR;
			}

			if ( nullptr == (to = sqlite3_column_text(stmt, 2)) )
			{
				PWLOGLIB("failed to get <to>");
				goto PROC_ERROR;
			}

			if ( nullptr == (value = sqlite3_column_text(stmt, 3)) )
			{
				PWLOGLIB("failed to get <value>");
				goto PROC_ERROR;
			}

			ins.begin = reinterpret_cast<const char*>(from);
			ins.end = reinterpret_cast<const char*>(to);
			ins.value = reinterpret_cast<const char*>(value);

			//PWTRACE("%s %s", ins.begin.c_str(), ins.end.c_str());

			switch(sqlite3_column_int(stmt, 0))
			{
			case 4: __insert(tmp4, ins); break;
#ifdef PW_HAVE_INT128
			case 6:	__insert(tmp6, ins); break;
#endif
			}//switch
		}

		if ( stmt )	sqlite3_finalize(stmt);
		if ( err ) sqlite3_free(err);
		if ( handle ) sqlite3_close(handle);

		tmp4.swap(m_cont4);
#ifdef PW_HAVE_INT128
		tmp6.swap(m_cont6);
#endif

		return true;

	} while (false);

PROC_ERROR:
	if ( stmt )	sqlite3_finalize(stmt);
	if ( err ) sqlite3_free(err);
	if ( handle ) sqlite3_close(handle);

	return false;
#else
	PWLOGLIB("not support sqlite3");
	return false;
#endif
}

bool
IpRange::readAsText (std::istream& is)
{
	v4_cont tmp4;
#ifdef PW_HAVE_INT128
	v6_cont tmp6;
#endif

	is.clear(is.rdstate());
	insert_type<std::string> ins;
	auto& line(ins.line);
	auto& line_count(ins.line_count);
	auto& begin(ins.begin);
	auto& end(ins.end);
	auto& value(ins.value);

	pw::string_list res;
	size_t res_count;

	while (std::getline(is, line).good())
	{
		++line_count;
		PWStr::trim(line);
		if (line.empty()) continue;

		auto c(line[0]);
		if ( c == '#' or c == ';' or c == '\'' ) continue;

		if ((res_count = PWStr::split2(res, line).size()) < 3)
		{
			PWLOGLIB("invalid skipped line: line:%zu buf:%s", line_count, line.c_str());
			continue;
		}

		auto res_itr(res.begin());
		const auto& type(*res_itr); ++res_itr;
		begin = *res_itr; ++res_itr;
		end = *res_itr; ++res_itr;
		if ( res_count > 3 ) value = *res_itr; else value.clear();

		if ( s_toIpVersion(type) == IpVersion::V4 )
		{
			__insert(tmp4, ins);
		}
#ifdef PW_HAVE_INT128
		else
		{
			__insert(tmp6, ins);
		}
#endif
	}// while

	m_cont4.swap(tmp4);
#ifdef PW_HAVE_INT128
	m_cont6.swap(tmp6);
#endif

	return true;
}

bool
IpRange::_read_json ( const void* json )
{
	//PWSHOWMETHOD();
#if HAVE_JSONCPP
	const Json::Value& root(*reinterpret_cast<const Json::Value*>(json));
	v4_cont tmp4;
#if PW_HAVE_INT128
	v6_cont tmp6;
#endif

	insert_type<std::string> ins;
	if ( root.isArray() )
	{
		//PWTRACE("array!");
		for ( auto& v : root )
		{
			//PWTRACE("new value!");
			++ins.line_count;
			if ( not v.isObject() )
			{
				PWTRACE("not object!");
				continue;
			}

			const Json::Value& type(v["type"]);
			if ( not type.isInt() )
			{
				PWTRACE("type is not int");
				continue;
			}

			const Json::Value& from(v["ip_begin"]);
			if ( not from.isString() )
			{
				PWTRACE("ip_begin is not string");
				continue;
			}

			const Json::Value& to(v["ip_end"]);
			if ( not to.isString() )
			{
				PWTRACE("ip_end is not string");
				continue;
			}

			const Json::Value& value(v["value"]);

			ins.begin = from.asString();
			ins.end = to.asString();
			if ( value.isString() ) ins.value = value.asString();
			else ins.value.clear();

			switch(type.asInt())
			{
			case 4: __insert(tmp4, ins); break;
#if PW_HAVE_INT128
			case 6: __insert(tmp6, ins); break;
#endif
			default:;
			}
		}
	}
	else
	{
		PWTRACE("not array!");
	}

	m_cont4.swap(tmp4);
#if PW_HAVE_INT128
	m_cont6.swap(tmp6);
#endif

	return true;
#else
	return false;
#endif
}


bool
IpRange::readAsJson ( std::istream& is, const char* rootname )
{
#if HAVE_JSONCPP
	Json::Value root;
	Json::Reader reader;
	if ( not reader.parse(is, root, false) )
	{
		PWLOGLIB("failed to parse json");
		return false;
	}

	if ( rootname )
	{
		if ( not root.isMember(rootname) )
		{
			PWLOGLIB("no member: %s", rootname);
			return false;
		}

		return _read_json(&root[rootname]);
	}

	return _read_json(&root);

#else
	PWLOGLIB("not support json");
	return false;
#endif
}


bool
IpRange::writeAsText (const char* fn) const
{
	std::ofstream ofs(fn);
	if ( ofs.is_open() ) return this->write(ofs);
	return false;
}

bool
IpRange::writeAsText (std::ostream& os) const
{
	for ( auto& ib : m_cont4 )
	{
		os << this->s_ipv4_str << ' ';
		ib.first.write(os);
		os << ' ' << ib.second;
		os << "\r\n";
	}

#ifdef PW_HAVE_INT128
	for ( auto& ib : m_cont6 )
	{
		os << this->s_ipv6_str << ' ';
		ib.first.write(os);
		os << ' ' << ib.second;
		os << "\r\n";
	}
#endif

	return true;
}

template<Json::Int ver, typename _ContType>
void
_writeJson(Json::Value& out, int& idx, const _ContType& cont)
{
	for ( auto& v : cont )
	{
		auto& ar(out[idx]);
		ar["type"] = ver;
		ar["ip_begin"] = v.first.str(0);
		ar["ip_end"] = v.first.str(1);
		ar["value"] = v.second;
		++idx;
	}
}

bool
IpRange::writeAsJson ( std::ostream& os, const char* rootname ) const
{
#if HAVE_JSONCPP
	Json::FastWriter writer;
	Json::Value root;
	int idx(0);

	_writeJson<4>(root, idx, m_cont4);
#if PW_HAVE_INT128
	_writeJson<6>(root, idx, m_cont6);
#endif

	os << writer.write(root);
	return true;
#else
	PWLOGLIB("not support json");
	return false;
#endif
}

bool
IpRange::writeAsJson ( const char* fn, const char* rootname ) const
{
	std::ofstream ofs(fn);
	if ( ofs.is_open() ) return this->writeAsJson(ofs, rootname);
	return false;
}

template<size_t ver, typename _ContType>
void
_insertSqlite(sqlite3* _handle, const _ContType& cont, const char* tablename)
{
	sqlite3* handle(reinterpret_cast<sqlite3*>(_handle));
	char* err(nullptr);
	std::string sql;

	for ( auto& v : cont )
	{
		sql = PWStr::formatS("INSERT INTO %s (type, ip_begin, ip_end, value) VALUES (%zu, '%s', '%s', '%s')",
							tablename,
							ver,
							v.first.str(0).c_str(),
							v.first.str(1).c_str(),
							v.second.c_str());
		if ( SQLITE_OK not_eq sqlite3_exec(handle, sql.c_str(), nullptr, nullptr, &err) )
		{
			PWLOGLIB("failed to insert: %s", err);
			sqlite3_free(err);
			err = nullptr;
		}
	}
}

bool
IpRange::writeAsSqlite ( const char* fn, const char* tablename ) const
{
#if HAVE_SQLITE3
	sqlite3* handle(nullptr);
	char* err(nullptr);

	if ( not tablename ) tablename = DEF_TABLE_NAME;

	do {
		if ( SQLITE_OK not_eq sqlite3_open(fn, &handle) )
		{
			PWLOGLIB("failed to open sqlite: %s", fn);
			break;
		}

		std::string sql(PWStr::formatS("DROP TABLE %s", tablename));
		sqlite3_exec(handle, sql.c_str(), nullptr, nullptr, nullptr);

		sql = PWStr::formatS("CREATE TABLE %s ("\
			"type INTEGER(1) NOT NULL,"\
			"ip_begin TEXT NOT NULL,"\
			"ip_end TEXT NOT NULL,"\
			"value TEXT NOT NULL"\
			")", tablename);

		if ( SQLITE_OK not_eq sqlite3_exec(handle, sql.c_str(), nullptr, nullptr, &err) )
		{
			PWLOGLIB("failed to create table: %s", err);
			break;
		}

		_insertSqlite<4>(handle, m_cont4, tablename);
#if PW_HAVE_INT128
		_insertSqlite<6>(handle, m_cont6, tablename);
#endif

		if ( err ) sqlite3_free(err);
		if ( handle ) sqlite3_close(handle);

		return true;
	} while (false);

	if ( err ) sqlite3_free(err);
	if ( handle ) sqlite3_close(handle);

	return false;
#else
	PWLOGLIB("not support sqlite3");
	return false;
#endif
}

bool
IpRange::dump(std::ostream& os) const
{
	for ( auto& ib : m_cont4 )
	{
		os << this->s_ipv4_str << ' ';
		ib.first.dump(os);
		os << ' ' << ib.second;
		os << "\r\n";
	}

#ifdef PW_HAVE_INT128
	for ( auto& ib : m_cont6 )
	{
		os << this->s_ipv6_str << ' ';
		ib.first.dump(os);
		os << ' ' << ib.second;
		os << "\r\n";
	}
#endif

	return true;
}

};//namespace pw
