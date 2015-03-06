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
 * \file pw_apnschannel.cpp
 * \brief Channel for Apple Push Notification Service
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "pw_apnschannel.h"
#include "pw_instance_if.h"

namespace pw {

void
ApnsChannel::hookReadPacket (const PacketInterface& pk, const char* body, size_t blen)
{
	this->updateLastReadTime();
	eventReadPacket(pk, body, blen);
}

bool
pw::ApnsChannel::checkPingTimeout (void)
{
	// 핑 체크
	if ( not InstanceInterface::s_inst ) return false;
	return (this->getDiffFromLastRead()) < InstanceInterface::s_inst->getTimeoutPing();
}

void
ApnsChannel::eventPingTimeout (void)
{
	auto diff(this->getDiffFromLastRead());
	PWLOGLIB("eventPingTimeout: diff:%jd this:%p type:%s", diff, this, typeid(*this).name());
	setExpired();
}

void
ApnsChannel::eventTimer (int id, void*)
{
	//PWSHOWMETHOD();
	if ( not isConnSuccess() ) return;

	if ( isInstDeleteOrExpired() ) return;

	if ( id == TIMER_CHECK_10SEC )
	{
		if ( not checkPingTimeout() ) eventPingTimeout();
	}
}

void
ApnsChannel::eventReadData (size_t len)
{
	ApnsResponsePacket rpk;
	//PWSHOWMETHOD();
	do {
		switch(m_recv_state)
		{
		case RecvState::START:
		{
			rpk.clear();
			m_recv_state = RecvState::HEADER;
		}// Fall to RecvState::HEADER
		/* no break */
		case RecvState::HEADER:
		{
			//PWTRACE("RecvState::HEADER: %p", this);
			if ( m_rbuf->getReadableSize() < 6/* rpk.getPacketSize() */ )
			{
				//PWTRACE("not yet: min:%jd", intmax_t(MsgPacket::limit_type::MIN_HEADER_SIZE) );
				// 아직 헤더 크기를 받지 못함.
				return;
			}

			IoBuffer::blob_type b;
			m_rbuf->grabRead(b);
			rpk.m_cmd = static_cast<ApnsPacket::Command>(b.buf[0]);
			rpk.m_status = static_cast<ApnsResponsePacket::Status>(b.buf[1]);
			memcpy(&(rpk.m_noti_id), &(b.buf[2]), sizeof(rpk.m_noti_id));
			m_rbuf->moveRead(6);
		}// Fall to RecvState::BODY
		/* no break */
		case RecvState::BODY:
		/* no break */
		case RecvState::DONE:
		{
			//PWTRACE("RecvState::DONE: %p", this);
			hookReadPacket(rpk, reinterpret_cast<const char*>(&rpk.m_noti_id), sizeof(rpk.m_noti_id));
			m_recv_state = RecvState::START;
			break;
		}// case RecvState::DONE
		case RecvState::ERROR:
		{
//PROC_ERROR:
			//PWTRACE("RecvState::ERROR: %p", this);
			eventError(Error::INVALID_PACKET, 0);
			m_recv_state = RecvState::START;
			if ( isInstDeleteOrExpired() ) return;
		}
		/* no break */
		default:
		{
			PWLOGLIB("Invalid state: %d", static_cast<int>(m_recv_state));
			m_recv_state = RecvState::START;
			break;
		}
		}// switch(m_recv_state)
	} while ( true );
}

}
