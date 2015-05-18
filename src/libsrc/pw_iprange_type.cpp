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
 * \file pw_iprange_type.cpp
 * \brief Support IP range for IPv4 and IPv6.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_iprange_type.h"
#include <arpa/inet.h>
#include <bitset>

namespace pw {

namespace iprange {

template<>
inline
bool _getData<uint32_t>(uint32_t& out, const SocketAddress& sa)
{
	if ( sa.getFamily() not_eq AF_INET ) return false;
	auto sin(reinterpret_cast<const sockaddr_in*>(sa.getData()));
	auto addr(reinterpret_cast<const uint32_t*>(&(sin->sin_addr)));
	out = *addr;
	return true;
}

#ifdef PW_HAVE_INT128
template<>
inline
bool _getData<uint128_t>(uint128_t& out, const SocketAddress& sa)
{
	if ( sa.getFamily() not_eq AF_INET6 ) return false;
	auto sin(reinterpret_cast<const sockaddr_in6*>(sa.getData()));
	auto addr(reinterpret_cast<const uint128_t*>(&(sin->sin6_addr)));
	out = *addr;
	return true;
}
#endif

template<>
bool _toValue<uint32_t>(uint32_t& out, const char* in)
{
	return ( 1 == inet_pton(AF_INET, in, &out) );
}

#ifdef PW_HAVE_INT128
template<>
bool _toValue<uint128_t>(uint128_t& out, const char* in)
{
	return ( 1 == inet_pton(AF_INET6, in, &out) );
}
#endif

template<>
std::string _toString<uint32_t>(uint32_t v)
{
	thread_local static char buf[INET_ADDRSTRLEN+1];
	inet_ntop(AF_INET, &v, buf, sizeof(buf));
	return std::string(buf);
}

#ifdef PW_HAVE_INT128
template<>
std::string _toString<uint128_t>(uint128_t v)
{
	thread_local static char buf[INET6_ADDRSTRLEN+1];
	inet_ntop(AF_INET6, &v, buf, sizeof(buf));
	return std::string(buf);
}
#endif

}; // namespace pw::iprange

};// namespace pw
