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
 * \file pw_redischannel.cpp
 * \brief Channel for Redis(http://redis.io/).
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_instance_if.h"
#include "./pw_redischannel.h"
#include "./pw_redispacket.h"

#define AS_CH(p) static_cast<RedisChannel*>(p)
#define AS_RD(p) static_cast<redisReader*>(p)

namespace pw {

void
RedisChannel::hookReadPacket(const PacketInterface& pk, const char* body, size_t blen)
{
	eventReadPacket(pk, body, blen);
}

bool
RedisChannel::checkPingTimeout(void)
{
	// 핑 체크
	if ( not InstanceInterface::s_inst ) return false;
	return (this->getDiffFromLastRead()) < InstanceInterface::s_inst->getTimeoutPing();
}

void
RedisChannel::eventPingTimeout(void)
{
	auto diff(this->getDiffFromLastRead());
	PWLOGLIB("eventPingTimeout: diff:%jd this:%p type:%s", diff, this, typeid(*this).name());
	setExpired();
}

void
RedisChannel::eventTimer(int id, void*)
{
	if ( not isConnSuccess() ) return;

	if ( isInstDeleteOrExpired() ) return;

	if ( id == TIMER_CHECK_10SEC )
	{
		if ( not checkPingTimeout() ) eventPingTimeout();
	}
}

void
RedisChannel::eventReadData(size_t len)
{
	if ( m_reader.parse(*m_rbuf) > 0 )
	{
		RedisResponsePacket rpk;
		while ( m_reader.pop(rpk.m_body) )
		{
			this->hookReadPacket(rpk, nullptr, 0);
		}
	}
}

//namespace pw
}
