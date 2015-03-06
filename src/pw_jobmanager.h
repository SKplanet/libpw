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
 * \file pw_jobmanager.h
 * \brief Support job(transaction).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_packet_if.h"
#include "./pw_channel_if.h"
#include "./pw_timer.h"

#ifndef __PW_JOBMANAGER_H__
#define __PW_JOBMANAGER_H__

namespace pw {

class JobManager;

using job_key_type = uint32_t;

//! \brief 트랜젝션 처리
class Job
{
public:
	inline Job(JobManager& man);

	Job() = delete;
	Job(const Job&) = delete;
	Job(Job&&) = delete;

	Job& operator = (const Job&) = delete;
	Job& operator = (Job&&) = delete;

public:
	//! \brief 잡 매니저 객체
	inline JobManager& getManager(void) { return m_man; }

	//! \brief 잡 매니저 객체
	inline const JobManager& getManager(void) const { return m_man; }

	//! \brief 잡 시작 시간
	inline int64_t getStart(void) const { return m_start; }

	//! \brief 걸린 시간
	inline int64_t getDiff(void) const { return (Timer::s_getNowMicro() - m_start); }

	//! \brief 잡 키
	inline job_key_type getKey(void) const { return m_key; }

	inline void setRelease(void);

protected:
	//! \brief 패킷을 받았을 경우, 잡 매니저에 의해 호출
	//! \param[inout] pch 채널.
	//! \param[in] pk 패킷. dynamic_cast로 캐스트 하여 사용할 것.
	//! \param[inout] param 파라매터.
	//! \param[out] del_this true일 경우, 잡 매니저가 delete를 호출한다.
	virtual void eventReadPacket(ChannelInterface* pch, const PacketInterface& pk, void* param, bool& del_this) { del_this = true; }

	//! \brief 잡 타임아웃이 발생했을 경우, 잡 매니저에 의해 호출
	//! \param[in] diff 걸린 시간
	//! \param[out] del_this true일 경우, 잡 매니저가 delete를 호출한다.
	virtual void eventTimeout(int64_t diff, bool& del_this) { del_this = true; }

	//! \brief 채널 오류(접속 끊김 등)가 발생했을 경우, 잡 매니저에 의해 호출
	//! \param[inout] pch 이벤트가 발생한 채널
	//! \param[in] type 이벤트 종류
	//! \param[out] del_this true일 경우, 잡 매니저가 delete를 호출한다.
	virtual void eventError(ChannelInterface* pch, ChannelInterface::Error type, int err, bool& del_this) { del_this = true; }

private:
	JobManager& m_man;		//!< 매니저
	const int64_t m_start;	//!< 시작시간 (ms)
	const job_key_type m_key;		//!< 키

protected:
	inline virtual ~Job();

	friend class pw::JobManager;
};

//! \brief 트랜젝션을 위한 Job 클래스를 관리하는 클래스
class JobManager final
{
public:
	using Job = pw::Job;

public:
	//! \brief 해당 잡에 패킷을 넘긴다.
	bool dispatchPacket(job_key_type key, ChannelInterface* pch, const PacketInterface& pk, void* param = nullptr);
	void reservePacket(job_key_type key, ChannelInterface* pch, std::shared_ptr<PacketInterface> sptr_pk, void* param = nullptr);

	//! \brief 채널 에러에 대한 이벤트를 넘긴다.
	bool dispatchError(job_key_type key, ChannelInterface* pch, ChannelInterface::Error type, int err);
	void reserveError(job_key_type key, ChannelInterface* pch, ChannelInterface::Error type, int err);

	//! \brief 타임아웃이 발생한 잡을 처리한다.
	//!	InstanceInterface 상속 객체의 eventTimer에서 호출하는 것이 좋다.
	//! \param[in] timeout 타임아웃 시간(단위: ms)
	size_t checkTimeout(int64_t timeout);

	//! \brief 잡을 찾는다.
	inline Job* find(job_key_type key) { auto ib(m_jobs.find(key)); return m_jobs.end() == ib ? nullptr : ib->second; }

	//! \brief 관리 잡 개수를 반환한다.
	inline size_t size(void) const { return m_jobs.size(); }

private:
	//! \brief 키를 발급한다.
	job_key_type getKey(void);
	size_t dispatchKill(void);
	size_t dispatchReserve(void);

	inline void add(Job* job) { m_jobs[job->m_key] = job; }
	inline void remove(Job* job) { m_kills.insert(job->m_key); }
	inline void remove(job_key_type key) { m_kills.insert(key); }

private:
	using job_cont = std::unordered_map<job_key_type, Job*>;
	using kill_cont = std::unordered_set<job_key_type>;


	struct reserve_type {
		enum { PACKET, ERROR } type { PACKET };
		job_key_type key { 0 };
		ch_name_type ch_name { 0 };
		std::shared_ptr<PacketInterface> pk { nullptr };
		mutable void* param { nullptr };
		struct {
			ChannelInterface::Error type { ChannelInterface::Error::NORMAL };
			int no { 0 };
		} error;
	};

	using reserve_cont = std::list<reserve_type>;

private:
	job_cont m_jobs;
	kill_cont m_kills;
	job_key_type m_key;

	std::mutex m_reserve_lock;
	reserve_cont m_reserve;

friend class pw::Job;
};

Job::Job(JobManager& man) : m_man(man), m_start(Timer::s_getNow()), m_key(man.getKey())
{
	m_man.m_jobs[m_key] = this;
}

Job::~Job()
{
	m_man.m_jobs.erase(this->m_key);
}

void
Job::setRelease ( void )
{
	m_man.m_kills.insert(m_key);
}


};

#endif//!__PW_JOBMANAGER_H__

