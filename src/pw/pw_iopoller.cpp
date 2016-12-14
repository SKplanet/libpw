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
 * \file pw_iobuffer.cpp
 * \brief I/O buffer for channel.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_iopoller.h"
#include "./pw_iopoller_select.h"
#include "./pw_iopoller_epoll.h"
#include "./pw_iopoller_kqueue.h"

namespace pw {

std::string
IoPoller::s_getMaskString(int flags)
{
	std::string result;

	if ( flags & POLLIN ) result += "POLLIN|";
	if ( flags & POLLPRI ) result += "POLLPRI|";
	if ( flags & POLLOUT ) result += "POLLOUT|";
	if ( flags & POLLERR ) result += "POLLERR|";
	if ( flags & POLLHUP ) result += "POLLHUP|";
	if ( flags & POLLNVAL ) result += "POLLNVAL|";

	if ( result.empty() ) result = "(null)";
	else result.resize(result.size()-1);

	return result;
}

IoPoller*
IoPoller::s_create(const char* type)
{
	IoPoller* poller(nullptr);

	if ( nullptr == type ) type = "auto";

	do {
#ifdef HAVE_SELECT
		if ( !strcasecmp("select", type) ) { poller = new IoPoller_Select(); break; }
#endif

#ifdef HAVE_EPOLL
		if ( !strcasecmp("epoll", type) ) { poller =  new IoPoller_Epoll(); break; }
#endif

#ifdef HAVE_KQUEUE
		if ( !strcasecmp("kqueue", type) ) { poller = new IoPoller_Kqueue(); break; }
#endif

	// by system default
#ifdef HAVE_EPOLL
		poller = new IoPoller_Epoll();
		break;
#endif

#ifdef HAVE_KQUEUE
		poller = new IoPoller_Kqueue();
		break;
#endif

#ifdef HAVE_SELECT
		poller = new IoPoller_Select();
		break;
#endif
	} while (false);

	if ( poller )
	{
		if ( !poller->initialize() )
		{
			delete poller;
			poller = nullptr;
		}
	}

	return poller;
}

IoPoller*
IoPoller::s_createFromEpollFD(int fd)
{
#ifdef HAVE_EPOLL
	IoPoller_Epoll* poll(new IoPoller_Epoll());
	do {
		if ( nullptr == poll ) break;
		if ( not poll->initialize(fd) ) break;
		return poll;
	} while (false);

	if ( poll ) delete poll;
#endif

	return nullptr;
}

IoPoller*
IoPoller::s_createFromKqueueFD(int fd)
{
#ifdef HAVE_KQUEUE
	IoPoller_Kqueue* poll(new IoPoller_Kqueue());
	do {
		if ( nullptr == poll ) break;
		if ( not poll->initialize(fd) ) break;
		return poll;
	} while (false);

	if ( poll ) delete poll;
#endif

	return nullptr;
}

void
IoPoller::s_release(IoPoller* poller)
{
	delete poller;
}

};//namespace pw
