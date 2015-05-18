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
 * \file pw_sockaddr.cpp
 * \brief Support socket address.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_sockaddr.h"
#include "./pw_log.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace pw {

const int SocketAddress::s_default_get_name_flag(NI_NUMERICHOST|NI_NUMERICSERV);

bool
SocketAddress::_setIP4(const char* host, const char* service)
{
	list_type lst;
	if ( !s_parseName(lst, host, service, PF_INET, SOCK_STREAM, 0) ) return false;

	struct sockaddr_in* sa((struct sockaddr_in*)m_sa);
	struct sockaddr_in* sa2((struct sockaddr_in*)(lst.front().getData()));
	sa->sin_family = PF_INET;
	if ( host ) sa->sin_addr = sa2->sin_addr;
	if ( service ) sa->sin_port = sa2->sin_port;

	m_slen = sizeof(struct sockaddr_in);

	return true;
}

bool
SocketAddress::_setIP6(const char* host, const char* service)
{
	list_type lst;
	if ( !s_parseName(lst, host, service, PF_INET6, SOCK_STREAM, 0) ) return false;

	struct sockaddr_in6* sa((struct sockaddr_in6*)m_sa);
	struct sockaddr_in6* sa2((struct sockaddr_in6*)(lst.front().getData()));
	sa->sin6_family = PF_INET;
	if ( host ) sa->sin6_addr = sa2->sin6_addr;
	if ( service ) sa->sin6_port = sa2->sin6_port;

	m_slen = sizeof(struct sockaddr_in6);

	return true;
}

bool
SocketAddress::s_parseName(list_type& out, const char* host, const char* service, int family, int socktype, int protocol)
{
	list_type tmp;

	struct addrinfo hints;
	struct addrinfo* res;
	memset(&hints, 0x00, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	int getaddrinfo_res;
	if ( 0 != (getaddrinfo_res = getaddrinfo(host, service, &hints, &res)) )
	{
		PWLOGLIB("getaddrinfo failed(%d): host:%s service:%s family:%d socktype:%d protocol:%d %s", getaddrinfo_res, host, service, family, socktype, protocol, gai_strerror(getaddrinfo_res));
		tmp.swap(out);
		return false;
	}

	struct addrinfo* ressave(res);
	while ( res )
	{
		tmp.push_back(SocketAddress(res->ai_addr, res->ai_addrlen));
		res = res->ai_next;
	}

	freeaddrinfo(ressave);
	tmp.swap(out);
	return true;
}

size_t
SocketAddress::recalculateSize(void)
{
	const struct sockaddr* sa((struct sockaddr*)m_sa);

	switch(sa->sa_family)
	{
	case PF_INET: m_slen = sizeof(struct sockaddr_in); break;
	case PF_INET6: m_slen = sizeof(struct sockaddr_in6); break;
	case PF_UNIX: m_slen = sizeof(struct sockaddr_un); break;
	default: m_slen = sizeof(m_sa);
	}

	return m_slen;
}

bool
SocketAddress::getName(char* host, size_t hlen, char* service, size_t slen, int* err, int flag) const
{
	int res( getnameinfo((struct sockaddr*)m_sa, m_slen, host, hlen, service, slen, flag) );
	if ( err ) *err = res;
	return ( 0 == res );
}

bool
SocketAddress::getName(std::string* host, std::string* port, int* err, int flag) const
{
	char _host[MAX_HOST_SIZE];
	char _port[MAX_SERVICE_SIZE];

	int res( getnameinfo((struct sockaddr*)m_sa, m_slen, _host, sizeof(_host), _port, sizeof(_port), flag) );
	if ( err ) *err = res;
	if ( 0 == res )
	{
		if ( host ) host->assign(_host);
		if ( port ) port->assign(_port);
		return true;
	}

	return false;
}

void
SocketAddress::clear(void)
{
	::memset(m_sa, 0x00, sizeof(m_sa));
	m_slen = 0;
}

void
SocketAddress::setFamily(int family)
{
	struct sockaddr* sa((struct sockaddr*)&m_sa[0]);
	sa->sa_family = family;
	//((struct sockaddr*)&m_sa[0])->sa_family = family;
}

int
SocketAddress::getFamily(void) const
{
	const struct sockaddr* sa((struct sockaddr*)&m_sa[0]);
	return sa->sa_family;
	//return ((struct sockaddr*)m_sa)->sa_family;
}

bool
SocketAddress::setIP(int family, const char* host, const char* service)
{
	switch(family)
	{
	case PF_INET: return _setIP4(host, service);
	case PF_INET6: return _setIP6(host, service);
	}

	PWLOGLIB("invalid family: family:%d host:%s service:%s", family, host, service);
	return false;
}

bool
SocketAddress::getIP(int* pfamily, char* host, char* service) const
{
	const struct sockaddr* sa((struct sockaddr*)m_sa);
	const int family(sa->sa_family);

	if ( ! (family == PF_INET || family == PF_INET6) )
	{
		PWLOGLIB("invalid family: family:%d host:%s service:%s", family, host, service);
		return false;
	}

	if ( pfamily ) *pfamily = family;

	size_t hlen(MAX_HOST_SIZE);
	size_t slen(MAX_SERVICE_SIZE);

	int res(getnameinfo(sa, m_slen, host, hlen, service, slen, (NI_NUMERICHOST|NI_NUMERICSERV)));

	if ( 0 != res )
	{
		PWLOGLIB("failed to getnameinfo(%d): %s", res, gai_strerror(res));
		return false;
	}

	return true;
}

int
SocketAddress::getPort(void) const
{
	switch(getFamily())
	{
	case PF_INET:
	{
		const struct sockaddr_in* sa((struct sockaddr_in*)m_sa);
		return ntohs(sa->sin_port);
	}
	case PF_INET6:
	{
		const struct sockaddr_in6* sa((struct sockaddr_in6*)m_sa);
		return ntohs(sa->sin6_port);
	}
	};

	PWTRACE("invalid family for getPort");
	return -1;
}

bool
SocketAddress::setPath(const char* path)
{
	struct sockaddr_un* sa((struct sockaddr_un*)m_sa);
	m_slen  = sizeof(struct sockaddr_un);
	sa->sun_family = PF_UNIX;
	snprintf(sa->sun_path, sizeof(sa->sun_path), "%s", path);
	return true;
}

bool
SocketAddress::getPath(char* path) const
{
	const struct sockaddr_un* sa((struct sockaddr_un*)m_sa);
	if ( sa->sun_family == PF_UNIX )
	{
		strncpy(path, sa->sun_path, sizeof(sa->sun_path));
		return true;
	}

	return false;
}

bool
SocketAddress::getPath(std::string& path) const
{
	const struct sockaddr_un* sa((struct sockaddr_un*)m_sa);
	if ( sa->sun_family == PF_UNIX )
	{
		path = sa->sun_path;
		return true;
	}

	return false;
}

char*
SocketAddress::getPath(void)
{
	struct sockaddr_un* sa((struct sockaddr_un*)m_sa);
	return ( sa->sun_family == PF_UNIX ? sa->sun_path : nullptr );
}

const char*
SocketAddress::getPath(void) const
{
	const struct sockaddr_un* sa((struct sockaddr_un*)m_sa);
	return ( sa->sun_family == PF_UNIX ? sa->sun_path : nullptr );
}

bool
SocketAddress::assignByPeer(int fd)
{
	socklen_t slen(sizeof(m_sa));
	if ( 0 == getpeername(fd, (struct sockaddr*)m_sa, &slen) )
	{
		m_slen = size_t(slen);
		return true;
	}

	PWLOGLIB("failed to getpeername(%d): fd:%d %s", errno, fd, strerror(errno));
	return false;
}

bool
SocketAddress::assignBySocket(int fd)
{
	socklen_t slen(sizeof(m_sa));
	if ( 0 == getsockname(fd, (struct sockaddr*)m_sa, &slen) )
	{
		m_slen = size_t(slen);
		return true;
	}

	PWLOGLIB("failed to getpeername(%d): fd:%d %s", errno, fd, strerror(errno));
	return false;
}

bool
SocketAddress::assign(const void* sa)
{
	const struct sockaddr* _sa((struct sockaddr*)sa);
	size_t slen(0);

	switch (_sa->sa_family)
	{
	case PF_INET: slen = sizeof(struct sockaddr_in); break;
	case PF_INET6: slen = sizeof(struct sockaddr_in6); break;
	case PF_UNIX: slen = sizeof(struct sockaddr_un); break;
	default:
		PWLOGLIB("invalid socket family: family:%d", _sa->sa_family);
		return false;
	}

	memcpy(m_sa, sa, slen);
	m_slen = slen;

	return true;
}

bool
SocketAddress::assign(const void* sa, size_t slen)
{
	if ( slen > MAX_STORAGE_SIZE || 0 == slen )
	{
		PWLOGLIB("invalid socket address size: slen:%zu", slen);
		return false;
	}

	memcpy(m_sa, sa, slen);
	m_slen = slen;

	return true;
}

bool
SocketAddress::assign(const SocketAddress& sa)
{
	memcpy(m_sa, sa.m_sa, sa.m_slen);
	m_slen = sa.m_slen;

	return true;
}

};//namespace pw

