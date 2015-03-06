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
 * \file pw_listener_if.cpp
 * \brief Listener classes.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_listener_if.h"
#include "./pw_instance_if.h"
#include "./pw_ssl.h"

#include <sys/types.h>
#include <sys/socket.h>

namespace pw {

std::ostream&
ListenerInterface::accept_type::dump(std::ostream& os) const
{
	std::string host, port;
	sa.getName(&host, &port);
	os << "Listener addr: " << static_cast<const void*>(lsnr) << std::endl;
	os << "Listener type: " << type << std::endl;
	os << "Accepted fd: " << fd << std::endl;
	os << "Peer addr: " << host << ':' << port << std::endl;
	os << "Ssl session: " << static_cast<const void*>(ssl) << std::endl;
	os << "Appendix: " << append << std::endl;

	return os;
}

class SslDummyChannel : public Socket
{
public:
	inline explicit SslDummyChannel(const ListenerInterface::accept_type& type) : Socket(type.fd, type.lsnr->getIoPoller()), m_param(type), m_ssl(type.ssl), m_release(false), m_succ(false)
	{
		PWTRACE("%s", __func__);
		setNonBlocking();

		int revent(0);
		m_ssl->setFD(m_fd);
		if ( m_ssl->accept(&revent) )
		{
			m_succ = true;
			m_release = true;
			setIoPollerMask(POLLOUT);
		}
		else if ( revent == 0 )
		{
			//m_succ = false;
			m_release = true;
			setIoPollerMask(POLLOUT);
		}
		else
		{
			setIoPollerMask(revent);
		}
	}

	virtual ~SslDummyChannel()
	{
		if ( m_ssl )
		{
			Ssl::s_release(m_ssl);
			m_ssl = nullptr;
		}

		if ( -1 == m_fd )
		{
			::close(m_fd);
			m_fd = -1;
		}
	}

	inline void setRelRet(bool succ)
	{
		m_release = true;
		m_succ = succ;
	}

private:
	void eventIo(int fd, int event, bool& del_event)
	{
		do {
			if ( m_release )
			{
				if ( m_succ )
				{
					removeFromIoPoller();
					if ( m_param.lsnr->eventAccept(m_param) )
					{
						m_ssl = nullptr;
						m_fd = -1;
					}
				}

				delete this;
				break;
			}
			else
			{
				int revent(0);
				if ( m_ssl->handshake(&revent) )
				{
					PWTRACE("handshaking done");
					setRelRet(true);
				}
				else if ( revent == 0 )
				{
					PWTRACE("handshaking error");
					setRelRet(false);
				}
				else
				{
					PWTRACE("handshaking... revent:%d", revent);
					setIoPollerMask(revent);
					break;
				}
			}
		} while (true);
	}

private:
	ListenerInterface::accept_type	m_param;

	Ssl*	m_ssl;
	bool	m_release;
	bool	m_succ;
};

ListenerInterface::ListenerInterface(IoPoller* poller) : Socket(-1, poller), m_auto_async(true), m_ssl_ctx(nullptr)
{
	PWTRACE("new listener: %p pid:%d", this, int(::getpid()));
}

ListenerInterface::~ListenerInterface()
{
	PWTRACE("destroy listener: %p pid:%d", this, int(::getpid()));
}

SslContext*
ListenerInterface::setSslContext(SslContext* ctx)
{
	std::swap(m_ssl_ctx, ctx);
	return ctx;
}

Ssl*
ListenerInterface::getNewSsl(SslContext* ctx)
{
	if ( nullptr == ctx )
	{
		if ( nullptr == m_ssl_ctx )
		{
			return nullptr;
		}

		ctx = m_ssl_ctx;
	}

	return Ssl::s_create(*ctx);
}

bool
ListenerInterface::open(const char* host, const char* service, int family/* = PF_INET*/, int socktype/* = SOCK_STREAM*/, int protocol/* = 0*/)
{
	SocketAddress::list_type lst;
	if ( !SocketAddress::s_parseName(lst, host, service, family, socktype, protocol) )
	{
		return false;
	}

	SocketAddress::list_type::const_iterator ib(lst.begin());
	SocketAddress::list_type::const_iterator ie(lst.end());

	while ( ib != ie )
	{
		if ( this->open(*ib, socktype, protocol) ) return true;
		++ib;
	}

	return false;
}

void
ListenerInterface::close(void)
{
	if ( m_fd != -1 )
	{
		if ( m_poller ) m_poller->remove(m_fd);

		::close(m_fd);

		m_fd = -1;
	}
}

bool
ListenerInterface::open(const SocketAddress& sa, int socktype, int protocol)
{
	if ( m_fd >= 0 )
	{
		PWLOGLIB("already fd:%d", m_fd);
		return false;
	}

	int fd(-1);
	const char* errpos(nullptr);
	do {
		if ( -1 == (fd = ::socket(sa.getFamily(), socktype, protocol)) )
		{
			errpos = "socket";
			break;
		}

		int opt(1);
		if ( -1 == ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) )
		{
			errpos = "setsockopt::REUSEADDR";
			break;
		}

		if ( -1 == ::bind(fd, (struct sockaddr*)sa.getData(), sa.getSize()) )
		{
			errpos = "bind";
			break;
		}

		if ( -1 == ::listen(fd, 1024) )
		{
			errpos = "listen";
			break;
		}

		s_setNonBlocking(fd);

		m_fd = fd;
		if ( m_poller ) m_poller->add(fd, this, POLLIN);

		char _host[SocketAddress::MAX_HOST_SIZE] = "unknown";
		char _service[SocketAddress::MAX_SERVICE_SIZE] = "unknown";

		sa.getName(_host, sizeof(_host), _service, sizeof(_service));
		PWLOGLIB("new listener: host:%s service:%s fd:%d family:%d socktype:%d protocol:%d", _host, _service, m_fd, sa.getFamily(), socktype, protocol);

		return true;
	} while (false);

	PWLOGLIB("failed to %s(%d): port:%d fd:%d family:%d socktype:%d protocol:%d %s", errpos, errno, sa.getPort(), fd, sa.getFamily(), socktype, protocol, strerror(errno));

	return false;
}

void
ListenerInterface::eventIo(int fd, int event, bool& del_event)
{
	accept_type param;
	param.lsnr = this;

	socklen_t slen(SocketAddress::MAX_STORAGE_SIZE);
	SocketAddress& sa(param.sa);
	int& cfd(param.fd);

	if ( -1 == (cfd = ::accept(m_fd, (struct sockaddr*)sa.getData(), &slen)) )
	{
		PWLOGLIB("failed to accept new client(%d): fd:%d %s", errno, m_fd, strerror(errno));
		return;
	}

	if ( m_auto_async )
	{
		Socket::s_setNonBlocking(cfd);
	}

	sa.recalculateSize();

	do {
		if ( m_ssl_ctx )
		{
			PWLOGLIB("m_ssl_ctx:%p, getSslContext():%p", m_ssl_ctx, getSslContext());
			if ( nullptr == (param.ssl = getNewSsl(getSslContext())) )
			{
				PWLOGLIB("failed to create ssl");
				break;
			}

			if ( not eventSetParameters(param) )
			{
				PWLOGLIB("failed to set parameters");
				break;
			}

			SslDummyChannel* pch(new SslDummyChannel(param));
			if ( nullptr == pch )
			{
				PWLOGLIB("failed to create ssl neogociate instance");
				break;
			}

			return;
		}

		if ( not eventAccept(param) )
		{
			PWLOGLIB("failed to eventAccept");
			break;
		}

		return;
	} while (false);

	if ( param.ssl ) Ssl::s_release(param.ssl);
	::close(cfd);

	return;
}

ParentListener::ParentListener(int type, IoPoller* poller) : ListenerInterface(poller), m_type(type)
{
}

ParentListener::~ParentListener()
{
}

int
ParentListener::getPipeFD(void) const
{
	static size_t index(-1);
	//if ( not InstanceInterface::m_inst ) return -1;
	InstanceInterface& inst(*InstanceInterface::s_inst);
	size_t child_count(inst.getChildCount());
	if ( 0 == child_count ) return -1;
	index = (index + 1) % child_count;
	InstanceInterface::child_type* ct(inst.getChildByIndex(index));
	if ( not ct ) return -1;
	return (ct->getFDByParent());
}

void
ParentListener::eventIo(int fd, int event, bool& del_event)
{
	PWTRACE("ParentListener::eventIo this:%p fd:%d event:%d pid:%d ppid:%d",  this, fd, event, int(::getpid()), int(::getppid()));
	accept_type param;
	param.lsnr = this;

	socklen_t slen(SocketAddress::MAX_STORAGE_SIZE);
	SocketAddress& sa(param.sa);
	int& cfd(param.fd);

	if ( -1 == (cfd = ::accept(m_fd, (struct sockaddr*)sa.getData(), &slen)) )
	{
		PWLOGLIB("failed to accept new client(%d): pid:%d fd:%d %s", errno, int(::getpid()), m_fd, strerror(errno));
		return;
	}

	if ( m_auto_async )
	{
		Socket::s_setNonBlocking(cfd);
	}

	sa.recalculateSize();
	int pipe_fd(getPipeFD());

	do {
		if ( -1 == pipe_fd )
		{
			PWLOGLIB("failed to get pipe fd");
			break;
		}

		if ( sizeof(m_type) not_eq Socket::s_sendMessage(pipe_fd, cfd, (char*)&m_type, sizeof(m_type)) )
		{
			PWLOGLIB("failed to send fd to child process");
			break;
		}

		if ( not eventAccept(param) ) break;

		// 차일드로 넘어간 FD에 대해선 ::close를 호출해야한다.
		// 그래야 차일드에서 ::close를 호출 할 때 비로소 리소스가 정리 될테니
		// 2013-05-21 LYB
		::close(cfd);

		return;
	} while(false);

	::close(cfd);
}

ChildListenerInterface::ChildListenerInterface(IoPoller* poller) : ListenerInterface(poller)
{
}

ChildListenerInterface::~ChildListenerInterface()
{
}

bool
ChildListenerInterface::open(int pipe_fd)
{
	if ( m_fd not_eq -1 ) close();

	if ( -1 == pipe_fd )
	{
		if ( -1 == (pipe_fd = getPipeFD()) )
		{
			PWLOGLIB("failed to open child listener. pipe_fd is -1. pid:%d ppid:%d", int(::getpid()), int(::getppid()));
			return false;
		}
	}

	if ( not m_poller->add(pipe_fd, this, POLLIN) )
	{
		PWLOGLIB("failed to open child listener. register poll-in. pid:%d ppid:%d pipe_fd:%d", int(::getpid()), int(::getppid()), pipe_fd);
		return false;
	}

	m_fd = pipe_fd;

	return true;
}

int
ChildListenerInterface::getPipeFD(void) const
{
	auto& inst(*InstanceInterface::s_inst);
	auto ct(inst.getChildSelf());
	return ct->getFDByChild();
}

void
ChildListenerInterface::eventIo(int fd, int event, bool& del_this)
{
	accept_type param;
	param.lsnr = this;

	const int pipe_fd(getPipeFD());
	int& cfd(param.fd);
	int type(LT_NONE);
	if ( sizeof(type) not_eq Socket::s_receiveMessage(pipe_fd, cfd, (char*)&type, sizeof(type)) )
	{
		PWLOGLIB("failed to get fd from socket pair");
		return;
	}

	if ( m_auto_async )
	{
		Socket::s_setNonBlocking(cfd);
	}

	SocketAddress& sa(param.sa);
	sa.assignByPeer(cfd);
	param.type = static_cast<listener_type>(type);

	do {
		if ( not eventSetParameters(param) )
		{
			PWLOGLIB("failed to setParameters: this:%p type:%s", this, typeid(*this).name());
			break;
		}

		if ( param.ssl )
		{
			SslDummyChannel* pch(new SslDummyChannel(param));
			if ( nullptr == pch )
			{
				PWLOGLIB("failed to create ssl neogociate instance");
				break;
			}

			return;
		}

		if ( not eventAccept(param) )
		{
			PWLOGLIB("failed to eventAccept: this:%p type:%s", this, typeid(*this).name());
			break;
		}

		return;
	} while (false);

	if ( param.ssl ) Ssl::s_release(param.ssl);
	::close(cfd);

	return;
}

};//namespace pw

