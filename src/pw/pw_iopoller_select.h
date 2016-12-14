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
 * \file pw_iopoller_select.h
 * \brief I/O poller implementation of select.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_iopoller.h"

#if defined(HAVE_SELECT)
#ifndef __PW_IOPOLLER_SELECT_H__
#define __PW_IOPOLLER_SELECT_H__

#include <sys/select.h>

namespace pw {

class IoPoller_Select : public IoPoller
{
public:
	bool add(int fd, Event* client, int mask) override;
	bool remove(int fd) override;
	bool setMask(int fd, int mask) override;
	bool orMask(int fd, int mask) override;
	bool andMask(int fd, int mask) override;
	ssize_t dispatch(int timeout_msec) override;
	inline Event* getEvent(int fd) override { return ( fd < FD_SETSIZE ? m_clients[fd].event : nullptr ); }

public:
	const char* getType(void) const override { return "select"; }

protected:
	bool initialize(void) override;
	void destroy(void) override;

protected:
	IoPoller_Select() {}
	~IoPoller_Select() {}

protected:
	event_type	m_clients[FD_SETSIZE];
	ssize_t		m_max_fd;
	fd_set		m_rfds;
	fd_set		m_wfds;

friend class IoPoller;
};

}; // namespace pw

#endif//!__PW_IOPOLLER_SELECT_H__
#endif//HAVE_SELECT
