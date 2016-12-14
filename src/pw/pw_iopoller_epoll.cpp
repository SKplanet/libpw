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
 * \file pw_iopoller_epoll.cpp
 * \brief I/O poller implementation of epoll(Linux).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_iopoller_epoll.h"
#include "./pw_log.h"

#ifdef HAVE_EPOLL

namespace pw {

bool
IoPoller_Epoll::add(int fd, Event* client, int mask)
{
	if ( -1 == m_epoll ) return false;

	event_type& et(m_clients[fd]);
	et.fd = fd;
	et.mask = mask;
	et.event = client;

	struct epoll_event ev;
	ev.events = mask;
	ev.data.ptr = &et;

	if ( epoll_ctl(m_epoll, EPOLL_CTL_ADD, fd, &ev) == -1 )
	{
		PWTRACE("failed to add event(%d): fd: %d event: %p mask: %d %s", errno, fd, et.event, et.mask, strerror(errno));
		return false;
	}

	return true;
}

bool
IoPoller_Epoll::remove(int fd)
{
	if ( -1 == m_epoll ) return false;

	event_type& et(m_clients[fd]);
	if ( epoll_ctl(m_epoll, EPOLL_CTL_DEL, fd, nullptr) == -1 )
	{
		return false;
	}

	et.event = nullptr;
	et.fd = -1;
	et.mask = 0;

	return true;
}

bool
IoPoller_Epoll::setMask(int fd, int mask)
{
	event_type& et(m_clients[fd]);

	struct epoll_event ev;
	ev.events = mask;
	ev.data.ptr = &et;

	if ( epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev) == -1 )
	{
		PWLOGLIB("failed to setMask: fd: %d event: %p mask: %d", fd, et.event, mask);
		return false;
	}

	et.mask = mask;
	return true;
}


bool
IoPoller_Epoll::orMask(int fd, int mask)
{
	event_type& et(m_clients[fd]);
	mask |= et.mask;

	struct epoll_event ev;
	ev.events = mask;
	ev.data.ptr = &et;

	if ( epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev) == -1 )
	{
		PWLOGLIB("failed to orMask: fd: %d event: %p mask(f): %d", fd, et.event, mask);
		return false;
	}

	et.mask = mask;
	return true;
}


bool
IoPoller_Epoll::andMask(int fd, int mask)
{
	event_type& et(m_clients[fd]);
	mask &= et.mask;

	struct epoll_event ev;
	ev.events = mask;
	ev.data.ptr = &et;

	if ( epoll_ctl(m_epoll, EPOLL_CTL_MOD, fd, &ev) == -1 )
	{
		PWLOGLIB("failed to andMask: fd: %d event: %p mask(f): %d", fd, et.event, mask);
		return false;
	}

	et.mask = mask;
	return true;
}

ssize_t
IoPoller_Epoll::dispatch(int timeout_msec)
{
	ssize_t ret(epoll_wait(m_epoll, m_events, MAX_EVENT_SIZE, timeout_msec));
	if ( -1 == ret )
	{
		// skip intterupt error.
		if ( errno != EINTR )
		{
			PWLOGLIB("epoll_wait error(%d): %s", errno, strerror(errno));
			return -1;
		}

		return 0;
	}

	if ( ret > 0 )
	{
		struct epoll_event* ib(m_events);
		struct epoll_event* ie(m_events+ret);
		bool del_event(false);

		event_type* e(nullptr);

		while ( ib not_eq ie )
		{
			e = (event_type*)ib->data.ptr;
			if ( e->event )
			{
				del_event = false;
				e->event->eventIo(e->fd, ib->events, del_event);
				if ( del_event )
				{
					this->remove(e->fd);
				}
			}
			else
			{
				PWLOGLIB("epoll_wait invalid client: fd:%d", e->fd);
			}

			++ib;
		}
	}

	return ret;
}

bool
IoPoller_Epoll::initialize(void)
{
#ifdef HAVE_EPOLL_CREATE1
	int epoll_fd(epoll_create1(EPOLL_CLOEXEC));
#else
	int epoll_fd(epoll_create(MAX_EVENT_SIZE));
#endif
	if ( -1 == epoll_fd )
	{
		PWLOGLIB("failed to initialize epoll: %s", strerror(errno));
		return false;
	}

	if ( m_epoll not_eq -1 )
	{
		destroy();
	}

	m_epoll = epoll_fd;
	memset(m_clients, 0x00, sizeof(m_clients));

	return true;
}

bool
IoPoller_Epoll::initialize(int efd)
{
	m_epoll = efd;
	memset(m_clients, 0x00, sizeof(m_clients));
	return true;
}

void
IoPoller_Epoll::destroy(void)
{
	if ( m_epoll not_eq -1 )
	{
		::close(m_epoll);
		m_epoll = -1;
	}
}

};//namespace pw;
#endif//HAVE_EPOLL
