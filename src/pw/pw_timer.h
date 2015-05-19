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
 * \file pw_timer.h
 * \brief Timer framework.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_TIMER_H__
#define __PW_TIMER_H__

namespace pw
{

//! \brief 타이머 클래스.
//! \warning 싱글톤 객체이다.
class Timer final
{
public:
	//! \brief 이벤트 클라이언트
	class Event
	{
	protected:
		explicit Event();
		virtual ~Event();

		//! \brief 이벤트를 처리하기 위해 상속한다.
		virtual void eventTimer(int id, void* param) = 0;

	public:
		inline bool addToTimer(int id, int64_t cycle, void* param = nullptr);
		inline void removeFromTimer(int id);

	friend class Timer;
	};

public:
	//! \brief 싱글톤 객체를 얻는다.
	inline static Timer& s_getInstance(void) { static Timer inst; return inst; }

	//! \brief 현재 시간을 밀리세컨드로 얻는다.
	static int64_t s_getNow(void);

	//! \brief 현재 시간을 마이크로세컨드로 얻는다.
	static int64_t s_getNowMicro(void);

	//! \brief 밀리세컨드를 struct timeval 객체로 변환한다.
	inline static struct timeval& s_toTimeval(struct timeval& tv, int64_t milsec)
	{
		tv.tv_sec = milsec / 1000LL;
		tv.tv_usec = milsec % 1000LL * 1000LL;
		return tv;
	}

	//! \brief 마이크로세컨드를 struct timeval 객체로 변환한다.
	inline static struct timeval& s_toTimevalMicro(struct timeval& tv, int64_t microsec)
	{
		tv.tv_sec = microsec / 1000000LL;
		tv.tv_usec = microsec % 1000000LL;
		return tv;
	}

public:
	//! \brief 타이머 이벤트를 검사한다.
	//! \return 발생한 이벤트 개수를 반환한다.
	size_t check(void);

	//! \brief 타이머 이벤트를 검사 목록에 추가한다.
	bool add(Event* e, int id, int64_t cycle, void* param = nullptr);

	//! \brief 타이머 이벤트를 검사 목록에서 제거한다.
	void remove(Event* e, int id);

	//! \brief 타이머 이벤트를 목록을 비운다.
	void clear(void);

private:
	//! \brief 이벤트 관리객체
	typedef struct event_type
	{
		void*	param;
		int64_t	cycle;
		int64_t start;

		inline event_type(void* _param, int64_t _cycle, int64_t _start) : param(_param), cycle(_cycle), start(_start) {}
	} event_type;

	using event_cont = std::map<int, event_type>;
	using client_cont = std::map<intptr_t, event_cont>;	//<! key is Timer::Event*

private:
	inline void invalidateIterator(void) { m_inv_itrs = true; }

private:
	client_cont	m_clients;
	int64_t		m_last_check;
	bool		m_inv_itrs;

private:
	explicit Timer() : m_last_check(s_getNow()), m_inv_itrs(false) {}
	virtual ~Timer() {}

friend class Event;
};

inline bool
Timer::Event::addToTimer (int id, int64_t cycle, void* param)
{
	return Timer::s_getInstance().add(this, id, cycle, param);
}

inline void
Timer::Event::removeFromTimer (int id)
{
	Timer::s_getInstance().remove(this, id);
}

inline
bool
TimerAdd(Timer::Event* e, int id, int64_t cycle, void* param = nullptr)
{
	return Timer::s_getInstance().add(e, id, cycle, param);
}

inline
void
TimerRemove(Timer::Event* e, int id)
{
	Timer::s_getInstance().remove(e, id);
}

};//namespace pw

extern pw::Timer&	PWTimer;

#endif//!__PW_TIMER_H__

