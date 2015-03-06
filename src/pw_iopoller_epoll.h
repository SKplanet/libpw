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
 * \file pw_iopoller_epoll.h
 * \brief I/O poller implementation of epoll(Linux).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_iopoller.h"

#if defined(HAVE_EPOLL)
#ifndef __PW_IOPOLLER_EPOLL_H__
#define __PW_IOPOLLER_EPOLL_H__

#include <sys/epoll.h>

namespace pw {

class IoPoller_Epoll : public IoPoller
{
public:
	enum
	{
		MAX_EPOLL_SIZE = 1000000LL,
		MAX_EVENT_SIZE = 1024,
	};

public:
	bool add(int fd, Event* client, int mask);
	bool remove(int fd);
	bool setMask(int fd, int mask);
	bool orMask(int fd, int mask);
	bool andMask(int fd, int mask);
	ssize_t dispatch(int timeout_msec);
	inline Event* getEvent(int fd) { return ( fd < MAX_EPOLL_SIZE ? m_clients[fd].event : nullptr ); }

public:
	const char* getType(void) const { return "epoll"; }

protected:
	bool initialize(void);
	bool initialize(int efd);

public:
	void destroy(void);

protected:
	IoPoller_Epoll() : m_epoll(-1) {}
	~IoPoller_Epoll() {}

protected:
	int					m_epoll;
	event_type			m_clients[MAX_EPOLL_SIZE];
	struct epoll_event	m_events[MAX_EVENT_SIZE];

friend class IoPoller;
};

}; // namespace pw

#endif//!__PW_IOPOLLER_EPOLL_H__
#endif//HAVE_EPOLL
