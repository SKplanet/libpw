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
 * \file pw_apnspacket.h
 * \brief Packet for Apple Push Notification Service
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "pw_common.h"
#include "pw_channel_if.h"
#include "pw_apnspacket.h"

#ifndef __PW_APNSCHANNEL_H__
#define __PW_APNSCHANNEL_H__

namespace pw {

class ApnsChannel : public ChannelInterface, public ChannelPingInterface, public pw::Timer::Event
{
public:
	enum
	{
		TIMER_CHECK_10SEC = 25000,	//!< 10초에 한 번씩 검사
	};

public:
	using ChannelInterface::ChannelInterface;
	using ChannelPingInterface::ChannelPingInterface;
	virtual ~ApnsChannel() = default;

protected:
	//! \brief 서비스 채널을 위한 eventReadPacket 호출 후크
	//!	어플리케이션에서 상속할 일 없음.
	void hookReadPacket(const PacketInterface& pk, const char* body, size_t blen) override;

	//! \brief 유휴 시간을 검사한다.
	//!	false를 반환할 경우 eventPingTimeout을 호출한다.
	bool checkPingTimeout(void) override;

	//! \brief 유휴 시간이 InstanceInterface::m_timeout.ping을 넘었을 경우,
	//!	호출된다.
	void eventPingTimeout(void) override;

protected:
	void eventTimer(int, void*) override;

protected:
	int64_t		m_last_sent;	//!< 마지막 패킷 보낸 시간

private:
	//! \brief 패킷 해석. 상속하지 말 것.
	void eventReadData(size_t len) override;
};

}

#endif//__PW_APNSCHANNEL_H__
