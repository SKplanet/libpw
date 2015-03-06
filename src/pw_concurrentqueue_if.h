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
 * \file pw_concurrentqueue.h
 * \brief Support concurrent queue for multi-threaded application.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_CONCURRENTQUEUE_H__
#define __PW_CONCURRENTQUEUE_H__

namespace pw {

//! \brief Concurrent Queue Template
//! \warning 이 클래스는 복제 불가능하며, 상속할 수도 없다.
template<typename _Type>
class ConcurrentQueueTemplate final
{
public:
	using value_type = _Type;						//!< Value type
	using reference_type = _Type&;					//!< Reference type
	using moveable_type = _Type&&;					//!< Movable type
	using pointer_type = _Type*;					//!< Pointer type
	using queue_type = std::list<value_type>;		//!< Queue container
	using callable = void (*) (ConcurrentQueueTemplate<_Type>&, void*);

	using lock_type = std::mutex;
	using timed_lock_type = std::timed_mutex;
	using ulock_type = typename std::unique_lock<timed_lock_type>;
	using cv_type = std::condition_variable_any;

private:
	mutable cv_type m_cv;
	mutable timed_lock_type m_lock;
	queue_type m_cont;
	size_t m_max_size = size_t(0);

public:
	ConcurrentQueueTemplate() = default;			//!< 기본 생성자만 제공한다.
	~ConcurrentQueueTemplate() = default;			//!< 기본 소멸자.

	ConcurrentQueueTemplate(ConcurrentQueueTemplate&&) = default;
	ConcurrentQueueTemplate& operator = (ConcurrentQueueTemplate&&) = default;

	// 암묵적인 복제가 일어나지 않도록 기본생성자들을 모두 제거한다.
	ConcurrentQueueTemplate(const ConcurrentQueueTemplate&) = delete;
	ConcurrentQueueTemplate& operator = (const ConcurrentQueueTemplate&) = delete;

public:
	//! \brief 큐에서 하나를 꺼내온다.
	//! 큐가 비어 있거나, 다른 스레드에서 사용하는 동안은 무기한 기다린다.
	inline value_type pop(void)
	{
		ulock_type locked(m_lock);
		while (m_cont.empty()) m_cv.wait(locked);
		auto ret = m_cont.front();
		m_cont.pop_front();
		return ret;
	}

	//! \brief 큐에서 하나를 꺼내온다.
	//! 큐가 비어 있거나, 다른 스레드에서 사용하는 동안은 무기한 기다린다.
	inline void pop(value_type& ret)
	{
		ulock_type locked(m_lock);
		while (m_cont.empty()) m_cv.wait(locked);
		ret = m_cont.front();
		m_cont.pop_front();
	}

	inline void pop(value_type&& ret)
	{
		ulock_type locked(m_lock);
		while (m_cont.empty()) m_cv.wait(locked);
		ret = m_cont.front();
		m_cont.pop_front();
	}

	//! \brief 큐에서 시간 내에 하나를 꺼내온다.
	//! 큐가 비어 있거나, 다른 스레드에서 사용하는 동안 시간을 검사한다.
	//! \return 시간을 넘겼을 경우, 거짓을 반환한다.
	bool popTimed(value_type& ret, unsigned long msec)
	{
		if ( not msec ) return popTry(ret);

		const decltype(chrono::system_clock::now()) dest_limit { chrono::system_clock::now() + std::chrono::milliseconds(msec) };

		if ( not m_lock.try_lock_until(dest_limit) ) return false;

		ulock_type locked(m_lock, std::adopt_lock);

		while (m_cont.empty())
		{
			if ( std::cv_status::timeout == m_cv.wait_until(locked, dest_limit) ) return false;
		}

		ret = m_cont.front();
		m_cont.pop_front();
		return true;
	}

	//! \brief 큐에서 하나를 꺼내온다.
	//! 큐가 비어 있거나, 다른 스레드에서 사용하면 바로 실패를 반환한다.
	bool popTry(value_type& ret)
	{
		ulock_type locked(m_lock, std::try_to_lock);
		if ( not locked.owns_lock() ) return false;

		if ( m_cont.empty() ) return false;

		ret = m_cont.front();
		m_cont.pop_front();
		return true;
	}

	//! \brief 큐에 하나를 넣는다.
	//! 성공하면, pop하는 스레드를 깨운다.
	bool push(const value_type& r)
	{
		ulock_type locked(m_lock);
		if ( m_max_size and (m_cont.size() >= m_max_size) ) return false;

		m_cont.push_back(r);
		locked.unlock();
		m_cv.notify_one();
		return true;
	}

	//! \brief 큐에 하나를 넣는다.
	//! 성공하면, pop하는 스레드를 깨운다.
	bool push(value_type&& r)
	{
		ulock_type locked(m_lock);
		if ( m_max_size and (m_cont.size() >= m_max_size) ) return false;

		m_cont.push_back(r);
		locked.unlock();
		m_cv.notify_one();
		return true;
	}

	//! \brief 큐에 하나를 넣는다.
	//! 성공하면, pop하는 스레드를 깨운다.
	bool push_front(const value_type& r)
	{
		ulock_type locked(m_lock);
		if ( m_max_size and (m_cont.size() >= m_max_size) ) return false;

		m_cont.push_front(r);
		locked.unlock();
		m_cv.notify_one();
		return true;
	}

	//! \brief 큐에 하나를 넣는다.
	//! 성공하면, pop하는 스레드를 깨운다.
	bool push_front(value_type&& r)
	{
		ulock_type locked(m_lock);
		if ( m_max_size and (m_cont.size() >= m_max_size) ) return false;

		m_cont.push_front(r);
		locked.unlock();
		m_cv.notify_one();
		return true;
	}

public:
	//! \brief 큐가 비어 있는지 확인한다.
	inline bool empty(void) const
	{
		ulock_type locked(m_lock);
		return m_cont.empty();
	}

	//! \brief 큐를 비운다.
	//! \warning pop하는 스레드를 깨우지 않는다.
	inline void clear(void)
	{
		ulock_type locked(m_lock);
		m_cont.clear();
	}

	//! \brief 큐 아이템 개수를 반환한다.
	inline auto size(void) const -> decltype(this->m_cont.size())
	{
		ulock_type locked(m_lock);
		return m_cont.size();
	}

	inline size_t getMaxSize(void) const
	{
		ulock_type locked(m_lock);
		return m_max_size;
	}

	inline void setMaxSize(size_t s)
	{
		ulock_type locked(m_lock);
		m_max_size = s;
	}

private:
	//inline static timed_lock_type& s_toTimedLock(lock_type& lock) { return reinterpret_cast<timed_lock_type&>(lock); }
	//inline static lock_type& s_toLock(timed_lock_type& lock) { return reinterpret_cast<lock_type&>(lock); }

}; //template class ConcurrentQueueTemplate

};//namespace pw

#endif//__PW_CONCURRENTQUEUE_H__
