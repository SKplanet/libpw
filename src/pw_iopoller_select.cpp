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
 * \file pw_iopoller_select.cpp
 * \brief I/O poller implementation of select.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_iopoller_select.h"
#include "./pw_log.h"

#if defined(HAVE_SELECT)
namespace pw {
bool
IoPoller_Select::initialize(void)
{
	m_max_fd = -1;

	FD_ZERO(&m_rfds);
	FD_ZERO(&m_wfds);

	memset(m_clients, 0x00, sizeof(m_clients));

	return true;
}

void
IoPoller_Select::destroy(void)
{
	m_max_fd = -1;
}

bool
IoPoller_Select::add(int fd, Event* client, int mask)
{
	if ( fd >= FD_SETSIZE )
	{
		PWLOGLIB("fd is out of range: fd: %d MAX: %d", fd, FD_SETSIZE);
		return false;
	}

	if ( mask&POLLIN ) FD_SET(fd, &m_rfds);
	if ( mask&POLLOUT ) FD_SET(fd, &m_wfds);

	event_type& e(m_clients[fd]);
	e.fd = fd;
	e.event = client;
	e.mask = mask;

	if ( m_max_fd < fd ) m_max_fd = fd;

	return true;
}

bool
IoPoller_Select::remove(int fd)
{
	if ( fd >= FD_SETSIZE )
	{
		PWLOGLIB("fd is out of range: fd: %d MAX: %d", fd, FD_SETSIZE);
		return false;
	}

	FD_CLR(fd, &m_rfds);
	FD_CLR(fd, &m_wfds);

	event_type& e(m_clients[fd]);
	e.fd = -1;
	e.event = nullptr;
	e.mask = 0;

	while ( (m_max_fd >= 0) && m_clients[m_max_fd].event == nullptr )
	{
		-- m_max_fd;
	}

	return true;
}

bool
IoPoller_Select::setMask(int fd, int mask)
{
	if ( fd >= FD_SETSIZE )
	{
		PWLOGLIB("fd is out of range: fd: %d MAX: %d", fd, FD_SETSIZE);
		return false;
	}

	if ( mask&POLLIN ) FD_SET(fd, &m_rfds);
	else FD_CLR(fd, &m_rfds);
	if ( mask&POLLOUT ) FD_SET(fd, &m_wfds);
	else FD_CLR(fd, &m_wfds);

	event_type& e(m_clients[fd]);
	e.mask = mask;
	return true;
}

bool
IoPoller_Select::orMask(int fd, int mask)
{
	if ( fd >= FD_SETSIZE )
	{
		PWLOGLIB("fd is out of range: fd: %d MAX: %d", fd, FD_SETSIZE);
		return false;
	}

	event_type& e(m_clients[fd]);
	mask |= e.mask;
	return this->setMask(fd, mask);
}

bool
IoPoller_Select::andMask(int fd, int mask)
{
	if ( fd >= FD_SETSIZE )
	{
		PWLOGLIB("fd is out of range: fd: %d MAX: %d", fd, FD_SETSIZE);
		return false;
	}

	event_type& e(m_clients[fd]);
	mask &= e.mask;
	return this->setMask(fd, mask);
}

ssize_t
IoPoller_Select::dispatch(int timeout_msec)
{
	struct timeval tv;
	tv.tv_sec = timeout_msec / 1000;
	tv.tv_usec = (timeout_msec % 1000) * 1000;

	fd_set rfds(m_rfds);
	fd_set wfds(m_wfds);

	ssize_t ret(select(m_max_fd+1, &rfds, &wfds, nullptr, &tv));
	if ( -1 == ret )
	{
		PWLOGLIB("select error(%d): %s", errno, strerror(errno));
		return -1;
	}

	int event(0);
	bool del_event(false);

	for ( int fd(0); fd <= m_max_fd; fd++ )
	{
		event = 0;
		if ( FD_ISSET(fd, &rfds) ) event |= POLLIN;
		if ( FD_ISSET(fd, &wfds) ) event |= POLLOUT;

		if ( event )
		{
			del_event = false;
			event_type& e(m_clients[fd]);
			e.event->eventIo(fd, event, del_event);
			if ( del_event ) remove(fd);
		}
	}

	return ret;
}

}; //namespace pw
#endif//HAVE_SELECT
