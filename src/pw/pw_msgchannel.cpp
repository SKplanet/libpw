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
 * \file pw_msgchannel.cpp
 * \brief Channel for pw default message.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_msgchannel.h"
#include "./pw_string.h"
#include "./pw_encode.h"
#include "./pw_instance_if.h"

namespace pw {

MsgChannel::MsgChannel(const chif_create_type& param) : ChannelInterface(param), m_dest_bodylen(0), m_recv_bodylen(0), m_last_sent(Timer::s_getNow())
{
}

MsgChannel::~MsgChannel()
{
}

void
MsgChannel::hookReadPacket(const PacketInterface& pk, const char* body, size_t blen)
{
	//PWSHOWMETHOD();
	this->updateLastReadTime();
	eventReadPacket(pk, body, blen);
}

void
MsgChannel::eventTimer(int id, void* param)
{
	//PWSHOWMETHOD();
	if ( not isConnSuccess() ) return;

	if ( isInstDeleteOrExpired() ) return;

	if ( id == TIMER_CHECK_10SEC )
	{
		if ( not checkPingTimeout() ) eventPingTimeout();
	}
}

bool
MsgChannel::getPacketSync(MsgPacket& out)
{
	std::string header;
	if ( not getLineSync(header, size_t(MsgPacket::limit_type::MAX_HEADER_SIZE)) )
	{
		PWLOGLIB("failed to get line sync");
		return false;
	}

	MsgPacket tmp;
	if ( not tmp.setHeader(header.c_str(), header.size()) )
	{
		PWLOGLIB("failed to set header");
		return false;
	}

	if ( tmp.getBodySize() )
	{
		blob_type& body(tmp.m_body);
		if ( not getDataSync(const_cast<char*>(body.buf), body.size) )
		{
			PWLOGLIB("failed to sync body");
			return false;
		}
	}

	out.swap(tmp);
	return true;
}

void
MsgChannel::eventReadData(size_t len)
{
	//PWSHOWMETHOD();
	do {
		switch(m_recv_state)
		{
		case RecvState::START:
		{
			//PWTRACE("RecvState::START: %p", this);
			m_recv.clear();
			m_recv_bodylen = 0;
			m_dest_bodylen = 0;
			m_recv_state = RecvState::HEADER;
		}// Fall to RecvState::HEADER
		/* no break */
		case RecvState::HEADER:
		{
			//PWTRACE("RecvState::HEADER: %p", this);
			if ( m_rbuf->getReadableSize() < size_t(MsgPacket::limit_type::MIN_HEADER_SIZE) )
			{
				//PWTRACE("not yet: min:%jd", intmax_t(MsgPacket::limit_type::MIN_HEADER_SIZE) );
				// 아직 헤더 크기를 받지 못함.
				return;
			}

			IoBuffer::blob_type b;
			m_rbuf->grabRead(b);
			const char* eol(PWStr::findLine(b.buf, b.size));
			if ( nullptr == eol )
			{
				if ( b.size > size_t(MsgPacket::limit_type::MAX_HEADER_SIZE) )
				{
					PWLOGLIB("too long header: input:%jd", intmax_t(b.size));
					m_recv_state = RecvState::ERROR;
					goto PROC_ERROR;
				}

				return;
			}

			const size_t cplen(eol - b.buf);
			if ( not m_recv.setHeader(b.buf, cplen) )
			{
				// invalid packet
				std::string out;
				PWEnc::encodeHex(out, b.buf, cplen);
				PWLOGLIB("invalid packet: ch:%p chtype:%s header:%s", this, typeid(*this).name(), out.c_str());

				m_rbuf->moveRead(cplen+2);

				m_recv_state = RecvState::ERROR;
				goto PROC_ERROR;
			}

			m_rbuf->moveRead(cplen+2);

			if ( (m_dest_bodylen = m_recv.getBodySize()) > 0 )
			{
				m_recv_state = RecvState::BODY;
			}
			else
			{
				//m_recv_state = RecvState::DONE;
				//PWTRACE("RecvState::DONE: %p", this);
				//eventReadPacket(m_recv, m_recv.m_body.buf, m_recv.m_body.size);
				hookReadPacket(m_recv, m_recv.m_body.buf, m_recv.m_body.size);
				m_recv_state = RecvState::START;
				break;
			}
		}// Fall to RecvState::BODY
		/* no break */
		case RecvState::BODY:
		{
			//PWTRACE("RecvState::BODY: %p", this);
			blob_type& body(m_recv.m_body);
			if ( body.buf == nullptr )
			{
				if ( not body.allocate(m_dest_bodylen) )
				{
					PWLOGLIB("not enough memory");
					m_recv_state = RecvState::ERROR;
					break;
				}
			}// if (body.buf)

			IoBuffer::blob_type b;
			m_rbuf->grabRead(b);
			const size_t cplen(std::min(b.size, (m_dest_bodylen - m_recv_bodylen)) );
			if ( cplen > 0 )
			{
				char* p(const_cast<char*>(body.buf) + m_recv_bodylen);
				::memcpy(p, b.buf, cplen);
				m_recv_bodylen += cplen;
				m_rbuf->moveRead(cplen);
			}

			// 아직 더 받아야할 패킷이 있으면 반환한다.
			if ( m_recv_bodylen not_eq m_dest_bodylen )
			{
				return;
			}

			m_recv_state = RecvState::DONE;
		}
		/* no break */
		case RecvState::DONE:
		{
			//PWTRACE("RecvState::DONE: %p", this);
			hookReadPacket(m_recv, m_recv.m_body.buf, m_recv.m_body.size);
			m_recv_state = RecvState::START;
			break;
		}// case RecvState::DONE
		case RecvState::ERROR:
		{
PROC_ERROR:
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

bool
MsgChannel::checkPingTimeout(void)
{
	// 핑 체크
	if ( not InstanceInterface::s_inst ) return false;
	return (this->getDiffFromLastRead()) < InstanceInterface::s_inst->getTimeoutPing();
}

void
MsgChannel::eventPingTimeout(void)
{
	auto diff(this->getDiffFromLastRead());
	PWLOGLIB("eventPingTimeout: diff:%jd this:%p type:%s", diff, this, typeid(*this).name());
	setExpired();
}

};//namespace pw
