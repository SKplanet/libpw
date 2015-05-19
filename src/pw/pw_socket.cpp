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
 * \file pw_socket.cpp
 * \brief Default socket class.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_socket.h"
#include "./pw_timer.h"
#include "./pw_log.h"

#include <unistd.h>
#ifdef HAVE_FCNTL_H
#	include <fcntl.h>
#endif
#include <netdb.h>
#include <netinet/tcp.h>

namespace pw {

typedef union cmsg_fd
{
	struct cmsghdr cmsg;
	char data[CMSG_SPACE(sizeof(int))];
} cmsg_fd;

inline
static
int64_t
_set_leftTimeout(int64_t in, int64_t term)
{
	if ( (in == 0) or (term >= in) ) return 0;
	return in - term;
}

static
bool
_try_connect(int& sockfd, const char* host, const char* service, int family, bool async)
{
	sockfd = -1;

	struct addrinfo hints;
	struct addrinfo* res(nullptr);
	memset(&hints, 0x00, sizeof(hints));

	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;

	int gai_res;
	if ( (gai_res = getaddrinfo(host, service, &hints, &res)) != 0 )
	{
		PWLOGLIB("failed to getaddrinfo(%d): host:%s service:%s %s", gai_res, host, service, gai_strerror(gai_res));
		return false;
	}

	struct addrinfo* ressave(res);

	if ( res == nullptr )
	{
		PWTRACE("res is null");
		return false;
	}

	int fd(-1);
	size_t retry(0);

	do {
		++ retry;

		if ( (fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0 )
		{
			PWLOGLIB("failed to create socket(%d): family:%d socktype:%d protocol:%d %s", errno, res->ai_family, res->ai_socktype, res->ai_protocol, strerror(errno));
			break;
		}

		if ( async )
		{
			if ( not Socket::s_setNonBlocking(fd) )
			{
				PWTRACE("set non-blocking mode failed");
				break;
			}
		}// if ( async )

		if ( ::connect(fd, res->ai_addr, res->ai_addrlen) == 0 )
		{
			::freeaddrinfo(ressave);

			sockfd = fd;
			return true;
		}

		// errno가 EINPROGRESS이면 비동기적으로 접속을 처리 중이니
		// 다음 쓰기 이벤트에서 처리할 수 있도록 한다.
		if ( (errno == EINPROGRESS) and async )
		{
			::freeaddrinfo(ressave);

			sockfd = fd;
			return false;
		}

		PWTRACE("connect failed but retry(%d): retry:%zu host:%s service:%s %s", errno, retry, host, service, strerror(errno));

		::close(fd);
		fd = -1;

	} while ( (res = res->ai_next) not_eq nullptr );

	::freeaddrinfo(ressave);

	return false;
}

bool
Socket::s_isConnected(int fd, int* err)
{
	int _err(0);
	socklen_t slen(sizeof(_err));
	if ( getsockopt(fd, SOL_SOCKET, SO_ERROR, &_err, &slen) < 0 )
	{
		if ( err ) *err = _err;
		return false;
	}
	else if ( _err )
	{
		if ( err ) *err = _err;
		return false;
	}

	return true;
}

bool
Socket::s_setNoDelay(int fd, bool nodelay)
{
	const int v(nodelay);
	return 0 == setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
}

bool
Socket::s_setSendBufferSize(int fd, size_t blen)
{
	if ( (blen < 0) or (blen > INT_MAX) ) { errno = EFAULT; return false; }
	const int v(blen);
	return 0 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &v, sizeof(v));
}

bool
Socket::s_setReceiveBufferSize(int fd, size_t blen)
{
	if ( (blen < 0) or (blen > INT_MAX) ) { errno = EFAULT; return false; }
	const int v(blen);
	return 0 == setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &v, sizeof(v));
}

bool
Socket::s_setKeepAlive(int fd, bool keepalive)
{
	const int v(keepalive);
	return 0 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v));
}

bool
Socket::s_setReuseAddress(int fd, bool reuse)
{
	const int v(reuse);
	return 0 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
}

bool
Socket::s_isNonBlocking(int fd)
{
#if defined(O_NONBLOCK)
	int flags(0);
	if ( -1 == (flags = fcntl(fd, F_GETFL, 0)) ) flags = 0;
	return ( O_NONBLOCK == (flags bitand O_NONBLOCK) );
#else
	return false;
#endif
}

bool
Socket::s_setNonBlocking(int fd, bool block)
{
	if ( false == block ) return s_setNonBlocking(fd);

#if defined(O_NONBLOCK)
	int flags(0);
	if ( -1 == (flags = fcntl(fd, F_GETFL, 0)) ) flags = 0;

	flags &= ~O_NONBLOCK;
	if ( -1 == fcntl(fd, F_SETFL, flags) )
	{
		PWLOGLIB("failed to fcntl(%d): fd:%d %s", errno, fd, strerror(errno));
		return false;
	}
#else
	int flags(0);
	if ( -1 == ioctl(fd, FIONBIO, &flags) )
	{
		PWLOGLIB("failed to fcntl(%d): fd:%d %s", errno, fd, strerror(errno));
		return false;
	}
#endif

	return true;
}

bool
Socket::s_setNonBlocking(int fd)
{
#if defined(O_NONBLOCK)
	int flags(0);
	if ( -1 == (flags = fcntl(fd, F_GETFL, 0)) ) flags = 0;
	if ( -1 == fcntl(fd, F_SETFL, flags | O_NONBLOCK) )
	{
		PWLOGLIB("failed to fcntl(%d): fd:%d %s", errno, fd, strerror(errno));
		return false;
	}
#else
	int flags(1);
	if ( -1 == ioctl(fd, FIONBIO, &flags) )
	{
		PWLOGLIB("failed to fcntl(%d): fd:%d %s", errno, fd, strerror(errno));
		return false;
	}
#endif

	return true;
}

ssize_t
Socket::s_receiveMessage(int pipe_fd, int& target_fd, char* buf, size_t blen)
{
	struct msghdr msg;
    struct iovec iov;
    cmsg_fd cmsg;
    target_fd = -1;

    iov.iov_base = buf;
    iov.iov_len = blen;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = &cmsg;
    msg.msg_controllen = sizeof(cmsg);

    msg.msg_flags = 0;

	ssize_t res(::recvmsg(pipe_fd, &msg, 0) );
	if ( -1 == res )
    {
		PWLOGLIB("failed to s_receiveMessage(%d): %s", errno, strerror(errno));
        return -1;
    }

    const struct cmsghdr* cptr(CMSG_FIRSTHDR(&msg));
    if ( nullptr == cptr )
    {
		PWLOGLIB("no cmsg 1st header: pid:%d", int(getpid()));
        return -1;
    }

    if ( SOL_SOCKET not_eq cptr->cmsg_level or SCM_RIGHTS not_eq cptr->cmsg_type )
    {
		PWLOGLIB("invalid control message");
        return -1;
    }

    memcpy(&target_fd, CMSG_DATA(cptr), sizeof(target_fd));
    return res;
}

ssize_t
Socket::s_sendMessage(int pipe_fd, int target_fd, const char* buf, size_t blen)
{
	struct msghdr msg;
    struct iovec iov;
    cmsg_fd cmsg;

    iov.iov_base = (void*)buf;
    iov.iov_len = blen;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = &cmsg;
    msg.msg_controllen = sizeof(cmsg);

    msg.msg_flags = 0;

    struct cmsghdr* cptr(CMSG_FIRSTHDR(&msg));

    // 이럴 리 없겠지?
    // if ( nullptr == cptr ) { 에러처리; }

    cptr->cmsg_len = CMSG_LEN(sizeof(int));
    cptr->cmsg_level = SOL_SOCKET;
    cptr->cmsg_type = SCM_RIGHTS;

    memcpy(CMSG_DATA(cptr), &target_fd, sizeof(target_fd));

	ssize_t res(::sendmsg (pipe_fd, &msg, 0) );
	if ( -1 == res )
    {
		PWLOGLIB("failed to s_sendMessage(%d): %s", errno, strerror(errno));
		return -1;
    }

    return res;
}

bool
Socket::s_connect(int& sockfd, const char* host, const char* service, int family, bool async)
{
	return _try_connect(sockfd, host, service, family, async);
}

bool
Socket::s_connectSync(int& sockfd, const char* host, const char* service, int family, size_t to_msec)
{
	int fd(-1);
	if ( not _try_connect(fd, host, service, family, true) )
	{
		if ( EINPROGRESS not_eq errno )
		{
			if ( fd not_eq -1 ) ::close(fd);
			return false;
		}

		struct timeval tv = { long(to_msec / 1000L), long(to_msec % 1000L * 1000L) };
		int res;
		fd_set wfd;

		do {
			FD_ZERO(&wfd);
			FD_SET(fd, &wfd);

			if ( 0 < (res = select(fd+1, nullptr, &wfd, nullptr, &tv)) )
			{
				break;
			}
			else if ( res == 0 )
			{
				errno = ETIMEDOUT;
				::close(fd);
				return false;
			}
			else if ( (errno == EINTR) or (errno == EAGAIN) )
			{
				continue;
			}
			else
			{
				::close(fd);
				return false;
			}
		} while (true);
	}

	if ( not s_setNonBlocking(fd, true) )
	{
		::close(fd);
		return false;
	}

	sockfd = fd;
	return true;
}

bool
Socket::s_connect(connect_param_type& param)
{
	const host_type& host(param.in.host);
	const bool async(param.in.async);

	const int64_t to_start(Timer::s_getNow());
	int fd(-1);
	errno = 0;

	const bool res(_try_connect(fd, host.host.c_str(), host.service.c_str(), param.in.family, true));
	if ( res )
	{
		param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
		param.out.fd = fd;
		param.out.err = 0;
		if ( not async ) s_setNonBlocking(fd, true);
		return res;
	}
	else if ( async or (fd == -1) )
	{
		param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
		param.out.fd = fd;
		param.out.err = errno;
		return false;
	}

	// Sync socket routine
	if ( EINPROGRESS not_eq errno )
	{
		if ( fd not_eq -1 ) ::close(fd);

		param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
		param.out.fd = -1;
		param.out.err = ( errno not_eq 0 ) ? errno : ETIMEDOUT;

		return false;
	}

	struct timeval tv = { (param.in.timeout / 1000LL), (param.in.timeout % 1000LL) };
	struct timeval* ptv(param.in.timeout ? &tv : nullptr);
	fd_set wfd;
	int selfd(0);

	FD_ZERO(&wfd);

	do {
		FD_SET(fd, &wfd);
		if ( (selfd = select(fd+1, nullptr, &wfd, nullptr, ptv)) > 0 )
		{
			param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
			param.out.fd = fd;
			param.out.err = 0;

			s_setNonBlocking(fd, true);

			return true;
		}
		else if ( selfd == 0 )
		{
			errno = ETIMEDOUT;
			break;
		}
		else if ( (errno == EINTR) or (errno == EAGAIN) ) continue;
		else break;
	} while (true);

	if ( fd not_eq -1 ) ::close(fd);

	param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
	param.out.fd = -1;
	param.out.err = ( errno not_eq 0 ) ? errno : ETIMEDOUT;

	return false;
}

bool
Socket::s_receive(io_param_type& param)
{
	const bool async(param.in.async);
	char* buffer(const_cast<char*>(param.in.buffer));
	const size_t in_size(param.in.size);
	size_t& out_size(param.out.size);
	const int fd(param.in.fd);
	const int flag(param.in.flag);

	const int64_t to_start(Timer::s_getNow());
	errno = 0;

	ssize_t ret(-1);

	if ( async )
	{
		if ( (ret = ::recv(fd, buffer, in_size, flag)) > 0 )
		{
			out_size = static_cast<size_t>(ret);
			param.out.err = 0;
			param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
			return true;
		}

		out_size = 0;
		param.out.err = errno;
		param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
		return false;
	}

	// Sync-socket
	struct timeval tv = { (param.in.timeout / 1000LL), (param.in.timeout % 1000LL) };
	struct timeval* ptv(param.in.timeout ? &tv : nullptr);
	fd_set rfd;
	int selfd(0);

	FD_ZERO(&rfd);

	do {
		FD_SET(fd, &rfd);
		if ( (selfd = select(fd+1, &rfd, nullptr, nullptr, ptv)) > 0 )
		{
			if ( (ret = ::recv(fd, buffer, in_size, flag)) > 0 )
			{
				out_size = static_cast<size_t>(ret);
				param.out.err = 0;
				param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
				return true;
			}

			break;
		}
		else if ( selfd == 0 )
		{
			errno = ETIMEDOUT;
			break;
		}
		else if ( (errno == EINTR) or (errno == EAGAIN) ) continue;
		else break;
	} while (true);

	out_size = 0;
	param.out.err = errno;
	param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);

	return false;
}

bool
Socket::s_send(io_param_type& param)
{
	const bool async(param.in.async);
	const char* buffer(param.in.buffer);
	const size_t in_size(param.in.size);
	size_t& out_size(param.out.size);
	const int fd(param.in.fd);
	const int flag(param.in.flag);

	const int64_t to_start(Timer::s_getNow());
	errno = 0;

	ssize_t ret(-1);

	if ( async )
	{
		if ( (ret = ::send(fd, buffer, in_size, flag)) > 0 )
		{
			out_size = static_cast<size_t>(ret);
			param.out.err = 0;
			param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
			return true;
		}

		out_size = 0;
		param.out.err = errno;
		param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
		return false;
	}

	// Sync-socket
	struct timeval tv = { (param.in.timeout / 1000LL), (param.in.timeout % 1000LL) };
	struct timeval* ptv(param.in.timeout ? &tv : nullptr);
	fd_set wfd;
	int selfd(0);

	FD_ZERO(&wfd);
	const char* pbuf(buffer);
	size_t left_size(in_size);

	do {
		FD_SET(fd, &wfd);
		if ( (selfd = select(fd+1, nullptr, &wfd, nullptr, ptv)) > 0 )
		{
			if ( (ret = ::send(fd, pbuf, left_size, flag)) > 0 )
			{
				pbuf += ret;
				left_size -= ret;

				if ( left_size == 0 )
				{
					out_size = in_size;
					param.out.err = 0;
					param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
					return true;
				}

				continue;
			}

			out_size = in_size - left_size;
			param.out.err = errno;
			param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
			return true;
		}
		else if ( selfd == 0 )
		{
			errno = ETIMEDOUT;
			out_size = in_size - left_size;
			param.out.err = errno;
			param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);
			return false;
		}
		else if ( (errno == EINTR) or (errno == EAGAIN) ) continue;
		else break;
	} while (true);

	out_size = 0;
	param.out.err = errno;
	param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - to_start);

	return false;
}

Socket::Socket(int fd, IoPoller* poller) : m_fd(fd), m_poller(poller)
{
	if ( m_fd >= 0 )
	{
		if ( m_poller )
		{
			m_poller->add(m_fd, this, POLLIN);
			//PWTRACE("poller added: this:%p %s fd:%d", this, typeid(*this).name(), m_fd);
		}
	}
}

Socket::~Socket()
{
	if ( m_fd != -1 )
	{
		if ( m_poller ) m_poller->remove(m_fd);

		// shutdown 하지 않는다.
		::close(m_fd);

		m_fd = -1;
	}
}

void
Socket::close(void)
{
	if ( m_fd != -1 )
	{
		if ( m_poller ) m_poller->remove(m_fd);

		::shutdown(m_fd, 2);
		::close(m_fd);

		m_fd = -1;
	}
}

bool
Socket::shutdown(Shutdown type)
{
	return (::shutdown(m_fd, static_cast<int>(type)) == 0);
}

}; //namespace pw

