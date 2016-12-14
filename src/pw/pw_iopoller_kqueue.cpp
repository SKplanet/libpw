/*
 * The MIT License (MIT)
 * Copyright (c) 2016 SK PLANET. All Rights Reserved.
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
 * \file pw_iopoller_kqueue.cpp
 * \brief I/O poller implementation of kqueue(BSD).
 * \copyright Copyright (c) 2016, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_iopoller_kqueue.h"
#include "./pw_log.h"
#include "./pw_string.h"

#ifdef HAVE_KQUEUE
namespace pw {

static
std::string
_evfilt2string(int evfilt)
{
	switch(evfilt)
	{
	case  EVFILT_READ: return std::string("EVFILT_READ");
	case  EVFILT_WRITE: return std::string("EVFILT_WRITE");
	case  EVFILT_AIO: return std::string("EVFILT_AIO");
	case  EVFILT_VNODE: return std::string("EVFILT_VNODE");
	case  EVFILT_PROC: return std::string("EVFILT_PROC");
	case  EVFILT_SIGNAL: return std::string("EVFILT_SIGNAL");
	case  EVFILT_TIMER: return std::string("EVFILT_TIMER");
	case  EVFILT_NETDEV: return std::string("EVFILT_NETDEV");
	case  EVFILT_FS: return std::string("EVFILT_FS");
	case  EVFILT_LIO: return std::string("EVFILT_LIO");
	case  EVFILT_USER: return std::string("EVFILT_USER");
	}

	return std::string("INVALID_EVFILT");
}

static
std::string
_evaction2string(unsigned int action)
{
	std::string result;
	if ( action & EV_ADD ) result += "EV_ADD|";
	if ( action & EV_DELETE ) result += "EV_DELETE|";
	if ( action & EV_ENABLE ) result += "EV_ENABLE|";
	if ( action & EV_DISABLE ) result += "EV_DISABLE|";

	if ( action & EV_ONESHOT ) result += "EV_ONESHOT|";
	if ( action & EV_CLEAR ) result += "EV_CLEAR|";
	if ( action & EV_RECEIPT ) result += "EV_RECEIPT|";
	if ( action & EV_DISPATCH ) result += "EV_DISPATCH|";

	if ( action & EV_SYSFLAGS ) result += "EV_SYSFLAGS|";
	if ( action & EV_FLAG1 ) result += "EV_FLAG1|";

	if ( action & EV_EOF ) result += "EV_EOF|";
	if ( action & EV_ERROR ) result += "EV_ERROR|";

	if ( result.empty() ) result = "(null)";
	else result.resize(result.size()-1);

	return result;
}

#if 0
static
std::string
_printKevent(const struct kevent& ev)
{
	std::string result;
	return PWStr::format(result, "ident: %d filter: %d %s flags: %u %s fflags: %u %s data: %" PRIxPTR " udata:%p",
		int(ev.ident),
		int(ev.filter), _evfilt2string(ev.filter).c_str(),
		(unsigned int)ev.flags, _evaction2string(ev.flags).c_str(),
		(unsigned int)ev.fflags, _evaction2string(ev.fflags).c_str(),
		ev.data,
		ev.udata);
}
#endif

inline
static
int
_kq2poll(int kev)
{
	switch(kev)
	{
	case EV_ERROR: return POLLERR;
	case EVFILT_READ: return POLLIN;
	case EVFILT_WRITE: return POLLOUT;
	}

	return POLLERR;
}

bool
IoPoller_Kqueue::initialize(void)
{
	int kfd = kqueue();
	if ( -1 == kfd ) return false;

	if ( not this->initialize(kfd) )
	{
		::close(kfd);
		return false;
	}

	return true;
}

bool
IoPoller_Kqueue::initialize(int kfd)
{
	if ( m_kqueue != -1 ) this->destroy();

	memset(m_clients, 0x00, sizeof(m_clients));

	do {
#ifdef OS_APPLE
		struct kevent ev, rev;
		memset(&ev, 0x00, sizeof(ev));
		ev.ident = -1;
		ev.filter = EVFILT_READ;
		ev.flags = EV_ADD;

		struct timespec ts = {0, 0};

		int kevent_res;
		const auto res_kevent( (kevent_res = kevent(kfd, &ev, 1, &rev, 1, &ts)) not_eq 1);

		if ( res_kevent )
		{
			PWTRACE("failed to initialize kqueue: res is not 1. %d", kevent_res);
			break;
		}

		const auto res_ident(rev.ident not_eq (uintptr_t)-1);
		if ( res_ident )
		{
			PWTRACE("failed to initialize kqueue: rev.ident is not -1. rev.ident: %d", rev.ident);
			break;
		}

		const auto res_flags(not (rev.flags bitand EV_ERROR));
		if ( res_flags )
		{
			PWTRACE("failed to initialize kqueue: rev.flags without EV_ERROR. rev.flags: %s", pw::c_str(_evaction2string(ev.flags)));
			break;
		}
#endif

		m_kqueue = kfd;
		return true;
	} while (false);

	return false;
}

void
IoPoller_Kqueue::destroy(void)
{
	if ( m_kqueue != -1 )
	{
		int fd = m_kqueue;
		m_kqueue = -1;
		::close(fd);
	}
}

bool
IoPoller_Kqueue::add(int fd, pw::IoPoller::Event *client, int mask)
{
	if ( m_kqueue == -1 ) return false;

	event_type& et(m_clients[fd]);
	if ( not setMask(fd, mask) )
	{
		PWTRACE("failed to add event(%d): fd: %d event: %p mask: %s %s", errno, fd, et.event, s_getMaskString(mask).c_str(), strerror(errno));
		return false;
	}

	et.event = client;
	return true;
}

bool
IoPoller_Kqueue::remove(int fd)
{
	if ( m_kqueue == -1 ) return false;

	event_type& et(m_clients[fd]);
	setMask(fd, 0);

	et.event = nullptr;
	et.fd = -1;
	et.mask = 0;

	return true;
}

bool
IoPoller_Kqueue::setMask(int fd, int mask)
{
	if ( -1 == m_kqueue ) return false;

	event_type& et(m_clients[fd]);

	bool io_read(mask bitand POLLIN);
	bool io_write(mask bitand POLLOUT);

	struct kevent krw[2], kres[2];
	memset(krw, 0x00, sizeof(krw));
	krw[0].ident = krw[1].ident = fd;
	krw[0].filter = EVFILT_WRITE;
	krw[1].filter = EVFILT_READ;
	krw[0].flags = krw[1].flags = EV_DELETE;
	krw[0].udata = krw[1].udata = &et;

	struct timespec ts = {0, 0};
	int kevent_res;

	if ( (kevent_res = kevent(m_kqueue, &krw[0], 2, &kres[0], 2, &ts)) < 0 )
	{
		PWLOGLIB("failed to setMask(delete): fd: %d event: %p mask: %d %s %s", fd, et.event, mask, s_getMaskString(mask).c_str(), strerror(errno));
		return false;
	}

	if ( io_write or io_read )
	{
		ts.tv_nsec = 0;
		ts.tv_sec = 0;
		if ( io_write ) krw[0].flags = EV_ENABLE | EV_ADD;
		if ( io_read ) krw[1].flags = EV_ENABLE | EV_ADD;
		if ( (kevent_res = kevent(m_kqueue, &krw[0], 2, &kres[0], 2, &ts)) < 0 )
		{
			PWLOGLIB("failed to setMask(add): fd: %d event: %p mask: %d %s %s", fd, et.event, mask, s_getMaskString(mask).c_str(), strerror(errno));
			return false;
		}
	}

	et.mask = mask;

	if ( io_write ) send(fd, nullptr, 0, 0);
	return true;
}

bool
IoPoller_Kqueue::orMask(int fd, int mask)
{
	event_type& et(m_clients[fd]);
	mask |= et.mask;
	return setMask(fd, mask);
}

bool
IoPoller_Kqueue::andMask(int fd, int mask)
{
	event_type& et(m_clients[fd]);
	mask &= et.mask;
	return setMask(fd, mask);
}

ssize_t
IoPoller_Kqueue::dispatch(int timeout_msec)
{
	struct timespec ts;
	ts.tv_sec = timeout_msec / 1000LL;
	ts.tv_nsec = (timeout_msec % 1000LL) * 1000000LL;

	ssize_t ret(kevent(m_kqueue, nullptr, 0, m_events, MAX_EVENT_SIZE, &ts));
	if ( ret < 0 )
	{
		// skip intterupt error.
		if ( errno not_eq EINTR and errno not_eq ENOENT )
		{
			PWLOGLIB("kevent error(%d): %s", errno, strerror(errno));
			return -1;
		}

		return 0;
	}

	struct kevent* ib(m_events);
	struct kevent* ie(m_events + ret);
	bool del_event(false);

	event_type* e(nullptr);

	if ( ret not_eq 0 )
	{
		while ( ib not_eq ie )
		{
			e = static_cast<event_type*>(ib->udata);
			if ( e->event )
			{
				del_event = false;
				e->event->eventIo(e->fd, _kq2poll(ib->filter), del_event);
				if ( del_event )
				{
					this->remove(e->fd);
				}
			}
			else
			{
				PWLOGLIB("kevent invalid client: fd:%d", e->fd);
			}

			++ib;
		}
	}

	return ret;
}
}//namespace pw
#endif//HAVE_KQUEUE
