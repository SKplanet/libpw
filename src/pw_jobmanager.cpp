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
 * \file pw_jobmanager.cpp
 * \brief Support job(transaction).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_jobmanager.h"
#include "./pw_timer.h"
#include "./pw_log.h"

namespace pw {

job_key_type
JobManager::getKey(void)
{
	job_key_type key(m_key+1);
	while ( (m_jobs.end() not_eq m_jobs.find(key)) or (0 == key) ) ++key;
	return (m_key = key);
}

size_t
JobManager::dispatchReserve(void)
{
	Job* pjob(nullptr);
	size_t count(0);
	for ( auto& r : m_reserve )
	{
		auto ib(m_jobs.find(r.key));
		if ( ib not_eq m_jobs.end() )
		{
			++count;
			pjob = ib->second;
			bool del_this (true);
			if ( r.type == reserve_type::PACKET )
			{
				pjob->eventReadPacket(ChannelInterface::s_getChannel(r.ch_name), getSafePacketInstance(r.pk.get()), r.param, del_this);
			}
			else
			{
				pjob->eventError(ChannelInterface::s_getChannel(r.ch_name), r.error.type, r.error.no, del_this);
			}

			if ( del_this ) delete pjob;
		}
	}

	m_reserve.clear();

	return count;
}

size_t
JobManager::dispatchKill(void)
{
	Job* pjob(nullptr);
	size_t count(0);
	for ( auto key : m_kills )
	{
		auto ib(m_jobs.find(key));
		if ( ib not_eq m_jobs.end() )
		{
			pjob = ib->second;
			m_jobs.erase(ib);
			if ( pjob )
			{
				PWTRACE("before DELETE");
				delete pjob;
				PWTRACE("after DELETE");
				++count;
			}
		}
	}

	//PWTRACE("clear kills");
	m_kills.clear();

	return count;
}

bool
JobManager::dispatchPacket(job_key_type key, ChannelInterface* pch, const PacketInterface& pk, void* param)
{
	Job* job(find(key));
	if ( nullptr == job ) return false;

	bool del_this(true);
	job->eventReadPacket(pch, pk, param, del_this);
	if ( del_this ) delete job;

	return true;
}
void
JobManager::reservePacket(job_key_type key, ChannelInterface* pch, std::shared_ptr<PacketInterface> sptr_pk, void* param)
{
	std::unique_lock<std::mutex>(m_reserve_lock);
	reserve_type tmp;
	tmp.type = reserve_type::PACKET;
	tmp.key = key;
	tmp.ch_name = pch?pch->getUniqueName():0;
	if ( sptr_pk ) tmp.pk = sptr_pk;
	if ( param ) tmp.param = param;
	this->m_reserve.push_back(tmp);
}

bool
JobManager::dispatchError(job_key_type key, ChannelInterface* pch, ChannelInterface::Error type, int err)
{
	Job* job(find(key));
	if ( nullptr == job ) return false;

	bool del_this(true);
	job->eventError(pch, type, err, del_this);
	if ( del_this ) delete job;

	return true;
}

void
JobManager::reserveError(job_key_type key, ChannelInterface* pch, ChannelInterface::Error type, int err)
{
	std::unique_lock<std::mutex>(m_reserve_lock);
	reserve_type tmp;
	tmp.type = reserve_type::ERROR;
	tmp.key = key;
	tmp.ch_name = pch?pch->getUniqueName():0;
	tmp.error.type = type;
	tmp.error.no = err;
	this->m_reserve.push_back(tmp);
}

size_t
JobManager::checkTimeout(int64_t timeout)
{
	size_t count(0);
	Job* pjob(nullptr);

	int64_t now(Timer::s_getNow());
	bool del_this;

	dispatchKill();
	dispatchReserve();

	auto ib(m_jobs.begin()), ie(m_jobs.end());
	while ( ib not_eq ie )
	{
		pjob = ib->second;
		++ib;

		if ( (now - pjob->m_start) > timeout )
		{
			++count;

			del_this = true;
			pjob->eventTimeout(now - pjob->m_start, del_this);
			if ( del_this ) delete pjob;
		}
	}

	return count;
}

};
