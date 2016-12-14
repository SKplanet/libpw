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
 * \file pw_iopoller.h
 * \brief I/O poller for multiplexing.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_IOPOLLER_H__
#define __PW_IOPOLLER_H__

#ifdef HAVE_POLL
#	include <sys/poll.h>
#else
#	define	POLLIN		0x001
#	define	POLLPRI		0x002
#	define	POLLOUT		0x004
#	define	POLLERR		0x008
#	define	POLLHUP		0x010
#	define	POLLNVAL	0x020
#endif

namespace pw {

//! \brief IO 폴러 인터페이스
class IoPoller
{
public:
	class Event;

	//! \brief 이벤트 내용
	typedef struct event_type
	{
		int 	fd;		//!< FD
		int		mask;	//!< Mask
		Event*	event;	//!< Event

		inline event_type() : fd(-1), mask(0), event(nullptr) {}
	} event_type;

	//! \brief 폴러 클라이언트
	class Event
	{
	public:
		virtual ~Event() {}

		//! \brief IO 이벤트.
		virtual void eventIo(int fd, int flags, bool& del_event) = 0;
	};

public:
	//! \brief 폴러 생성.
	static IoPoller* s_create(const char* type = nullptr);

	//! \brief epoll fd로부터 epoll 객체 생성
	static IoPoller* s_createFromEpollFD(int fd);

	//! \brief kqueue fd로부터 kqueue 객체 생성
	static IoPoller* s_createFromKqueueFD(int fd);

	//! \brief 폴러 반환
	static void s_release(IoPoller* poller);

	//! \brief 폴러 플래그를 문자열로 변환
	static std::string s_getMaskString(int mask);

public:
	//! \brief 이벤트 감시 추가
	virtual bool add(int fd, Event* e, int mask) = 0;

	//! \brief 이벤트 감시 제거
	virtual bool remove(int fd) = 0;

	//! \brief 이벤트 마스크 설정
	virtual bool setMask(int fd, int mask) = 0;

	//! \brief 이벤트 마스크 OR 연산
	virtual bool orMask(int fd, int mask) = 0;

	//! \brief 이벤트 마스크 AND 연산
	virtual bool andMask(int fd, int mask) = 0;

	//! \brief 이벤트 디스패치
	virtual ssize_t dispatch(int timeout_msec) = 0;

	//! \brief 해당 클라이언트 얻기
	virtual Event* getEvent(int fd) = 0;

public:
	//! \brief 폴러 타입 문자열
	virtual const char* getType(void) const = 0;

protected:
	virtual bool initialize(void) = 0;
	virtual void destroy(void) = 0;

protected:
	explicit IoPoller() {}
	virtual ~IoPoller() {}
};

}; //namespace pw

#endif//!__PW_IOPOLLER_H__

