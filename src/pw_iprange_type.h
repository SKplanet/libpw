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
 * \file pw_iprange_type.h
 * \brief Support IP range for IPv4 and IPv6.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_sockaddr.h"

#ifndef __PW_IPRANGE_TYPE_H__
#define __PW_IPRANGE_TYPE_H__

namespace pw
{
namespace iprange
{

template<typename _Type>
bool _getData(_Type&, const SocketAddress& sa) { return _Type(); }

template<> bool _getData<uint32_t>(uint32_t&, const SocketAddress&);
template<> bool _getData<uint128_t>(uint128_t&, const SocketAddress&);

template<typename _Type>
bool _toValue(_Type& out, const char* in) { return false; }

template<> bool _toValue<uint32_t>(uint32_t&, const char*);
template<> bool _toValue<uint128_t>(uint128_t &, const char*);

template<typename _Type>
std::string _toString(_Type value) { return std::string(); }

template<> std::string _toString<uint32_t>(uint32_t);
template<> std::string _toString<uint128_t>(uint128_t);

template<typename _Type>
std::string
_toDumpString(_Type v)
{
	union _splitor {
		_Type x;
		uint8_t u8[sizeof(_Type)];
	}& k(reinterpret_cast<_splitor&>(v));

	std::string ret;
	ret.reserve(sizeof(_Type)*2);
	char buf[2+1];

	for ( auto i : k.u8 )
	{
		snprintf(buf, sizeof(buf), "%02x", int(i));
		ret.append(buf, 2);
	}

	return ret;
}

template<typename _Type>
class ItemTemplate final
{
public:
	using value_type = _Type;

public:
	static bool s_toValue(value_type& out, const char* v)
	{
		if ( _toValue<value_type>(out, v) )
		{
			PW_CONV_NET_ENDIAN(out);
			return true;
		}

		return false;
	}
	static value_type s_toValue(value_type& out, const std::string& v)
	{
		if ( _toValue<value_type>(out, v.c_str()) )
		{
			PW_CONV_NET_ENDIAN(out);
			return true;
		}

		return false;
	}
	static bool s_toValue(value_type& out, const pw::SocketAddress& v)
	{
		if ( _getData(out, v) )
		{
			PW_CONV_NET_ENDIAN(out);
			return true;
		}

		return false;
	}
	static std::string s_toString(const value_type& v)
	{
		return _toString<value_type>(PW_SWAP_NET_ENDIAN(v));
	}

public:
	inline ItemTemplate() = default;
	inline ItemTemplate(const value_type& begin, const value_type& end) : m_begin(begin), m_end(end)
	{
		if ( begin > end ) std::swap(m_begin, m_end);
	}
	inline ItemTemplate(const char* begin, const char* end)
	{
		this->assign(begin, end);
	}
	inline ItemTemplate(const std::string& begin, const std::string& end) : m_begin(), m_end()
	{
		this->assign(begin, end);
	}
	inline ItemTemplate(const SocketAddress& begin, const SocketAddress& end)
	{
		this->assign(begin, end);
	}
	inline ItemTemplate(const ItemTemplate&) = default;
	inline ItemTemplate(ItemTemplate&&) = default;
	inline ItemTemplate& operator = (const ItemTemplate&) = default;
	inline ItemTemplate& operator = (ItemTemplate&&) = default;

public:
	inline void assign(const value_type& begin, const value_type& end)
	{
		if ( begin <= end )
		{
			m_begin = begin;
			m_end = end;
		}
		else
		{
			m_begin = end;
			m_end = begin;
		}
	}
	inline void assign(const char* begin, const char* end)
	{
		s_toValue(m_begin, begin);
		s_toValue(m_end, end);
		if ( m_begin > m_end ) std::swap(m_begin, m_end);
	}
	inline void assign(const std::string& begin, const std::string& end)
	{
		this->assign(begin.c_str(), end.c_str());
	}

//	void assignAsCIDR ( const char* base, size_t bit_count );
//	inline void assignAsCIDR (const std::string& base, size_t bit_count) { this->assignAsCIDR(base.c_str(), bit_count); }

	inline std::ostream& write(std::ostream& os, size_t idx) const
	{
		return os << s_toString(idx == 0 ? m_begin : m_end);
	}
	inline std::ostream& write(std::ostream& os) const
	{
		return os << s_toString(m_begin) << ' ' << s_toString(m_end);
	}
	inline std::ostream& dump(std::ostream& os) const
	{
		return os << _toDumpString<value_type>(PW_SWAP_NET_ENDIAN(m_begin)) << ' ' << _toDumpString<value_type>(PW_SWAP_NET_ENDIAN(m_end));
	}
	inline std::string str(void) const
	{
		return s_toString(m_begin) + std::string(" ") + s_toString(m_end);
	}
	inline std::string str(size_t idx) const
	{
		return s_toString(idx ? m_end : m_begin);
	}

public:
	inline bool operator < (const ItemTemplate& ip) const
	{
		return m_end < ip.m_begin;
	}
	inline bool operator <= (const ItemTemplate& ip) const
	{
		if ( operator < (ip) ) return true;
		return operator == (ip);
	}
	inline bool operator > (const ItemTemplate& ip) const
	{
		return m_begin > ip.m_end;
	}
	inline bool operator >= (const ItemTemplate& ip) const
	{
		if ( operator > (ip) ) return true;
		return operator == (ip);
	}
	inline bool operator == (const ItemTemplate& ip) const
	{
		return (m_begin <= ip.m_begin) and (m_end >= ip.m_end);
	}
	inline bool operator not_eq (const ItemTemplate& ip) const
	{
		return not operator == (ip);
	}

private:
	value_type		m_begin;
	value_type		m_end;
};//class ItemTemplate

using ItemIpV4 = ItemTemplate<uint32_t>;

#ifdef PW_HAVE_INT128
using ItemIpV6 = ItemTemplate<uint128_t>;
#endif

}; // namespace pw::iprange
};// namespace pw
#endif// __PW_IPRANGE_TYPE_H__
