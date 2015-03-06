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
 * \file pw_timer.cpp
 * \brief Timer framework.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_timer.h"
#include "./pw_log.h"

#include <sys/time.h>

pw::Timer&	PWTimer(pw::Timer::s_getInstance());

namespace pw {

Timer::Event::Event()
{
	Timer::s_getInstance().invalidateIterator();
}

Timer::Event::~Event()
{
	Timer::s_getInstance().invalidateIterator();
}

int64_t
Timer::s_getNow(void)
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return int64_t((tv.tv_sec * 1000LL) + (tv.tv_usec / 1000LL));
}

int64_t
Timer::s_getNowMicro(void)
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return int64_t((tv.tv_sec * 1000000LL) + (tv.tv_usec));
}

void
Timer::clear(void)
{
	m_clients.clear();
	m_inv_itrs = true;
}

size_t
Timer::check(void)
{
	//PWSHOWMETHOD();
	const int64_t	now(s_getNow());
	auto diff(now - m_last_check);
	if ( diff < 100LL )
	{
		//PWTRACE("skip too short: diff:%jd", intmax_t(diff));
		return 0;
	}

	m_last_check = now;

	auto ib = m_clients.begin();
	auto ie = m_clients.end();
	event_cont::iterator ib_event, ie_event;

	intptr_t		last_client;
	Event*			event{nullptr};

	size_t ret(0);

	m_inv_itrs = false;
	while ( ib not_eq ie )
	{
		last_client = ib->first;
		event = reinterpret_cast<Event*>(ib->first);

		ib_event = ib->second.begin();
		ie_event = ib->second.end();

		++ib;	// --- ib 증가!!

		while ( ib_event not_eq ie_event )
		{
			event_type& e(ib_event->second);
			if ( (now - (e.start + e.cycle)) >= 1000LL )
			{
				//PWTRACE("event: %p type: %s", event, typeid(*event).name());

				e.start = now;
				event->eventTimer(ib_event->first, e.param);

				++ret;
			}

			// Event 추가 삭제가 일어날 경우 발생한다.
			// 이터레이터를 재정비하고 루프를 다시 돈다.
			// 중단점 기준으로 돌기 때문에 큰 이슈는 없다.
			if ( m_inv_itrs )
			{
				m_inv_itrs = false;
				ib = m_clients.lower_bound(last_client);
				ie = m_clients.end();
				break;
			}

			++ib_event;
		}
	}

	return ret;
}

bool
Timer::add(Event* e, int id, int64_t cycle, void* param)
{
	const intptr_t key((intptr_t)e);
	auto ib = m_clients.find(key);

	if ( ib == m_clients.end() )
	{
		auto res = m_clients.insert(client_cont::value_type(key, event_cont()));
		if ( res.second == false )
		{
			PWLOGLIB("failed to insert timer event: e:%p id:%d cycle:%jd param:%p", e, id, intmax_t(cycle), param);
			return false;
		}

		event_cont& ec(res.first->second);

		// 이후 코드는 insert 성공 여부와 상관 없이 반복자가 무효하다.
		invalidateIterator();

		if ( false == ec.insert(event_cont::value_type(id, event_type(param, cycle, s_getNow()))).second )
		{
			PWLOGLIB("failed to insert timer event: e:%p id:%d cycle:%jd param:%p", e, id, intmax_t(cycle), param);
			m_clients.erase(ib);
			return false;
		}

		//PWTRACE("add new timer event: e:%p type:%s id:%d cycle:%jdms", e, typeid(*e).name(), id, cycle);
		return true;
	}

	auto& ec = ib->second;
	auto ib_event = ec.find(id);

	if ( ec.end() == ib_event )
	{
		if ( false == ec.insert(event_cont::value_type(id, event_type(param, cycle, s_getNow()))).second )
		{
			PWLOGLIB("failed to insert timer event: e:%p id:%d cycle:%jd param:%p", e, id, intmax_t(cycle), param);
			return false;
		}

		invalidateIterator();
		//PWTRACE("add new timer event: e:%p type:%s id:%d cycle:%jdms", e, typeid(*e).name(), id, cycle);
		return true;
	}

	// 반복자를 건들지 않았으므로 invalidateIterator를 호출하지 않는다.
	auto& et = ib_event->second;
	et.param = param;
	et.cycle = cycle;
	et.start = s_getNow();

	//PWTRACE("add new timer event: e:%p type:%s id:%d cycle:%jdms", e, typeid(*e).name(), id, cycle);
	return true;
}

void
Timer::remove(Event* e, int id)
{
	const auto key(reinterpret_cast<intptr_t>(e));
	auto ib(m_clients.find(key));
	if ( ib == m_clients.end() ) return;

	auto& ec(ib->second);

	do {
		auto ib_event(ec.find(id));
		if ( ib_event == ec.end() ) return;

		ec.erase(ib_event);
		if ( ec.empty() ) m_clients.erase(ib);
	} while (false);

	invalidateIterator();
}

/* namespace pw */
}
