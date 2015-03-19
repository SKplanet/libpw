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
 * \file main.cpp
 * \brief Example for echo server.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#define SERVICE_NAME "echo"
#define SERVICE_PORT "5000"

#include <pw/pwlib.h>
using namespace pw;

class ServiceChannel;
class AdminChannel;

// 에코 채널.
class ServiceChannel final : public ChannelInterface
{
public:
	// I love C++11!
	virtual ~ServiceChannel() = default;
	using ChannelInterface::ChannelInterface;

private:
	StlStringPacket m_recv;

private:
	// 프로토콜 파싱.
	void eventReadData(size_t size) override
	{
		IoBuffer::blob_type b;
		m_rbuf->grabRead(b);
		m_recv.m_body.assign(b.buf, b.size);
		m_rbuf->moveRead(b.size);

		this->eventReadPacket(m_recv, m_recv.m_body.c_str(), m_recv.m_body.size());
	}

	// 패킷 클래스로 넘겨 로직을 실행.
	void eventReadPacket(const PacketInterface& in, const char*, size_t) override
	{
		write(in);
	}
};

// 리스너
class ServiceListener final : public ListenerInterface
{
public:
	virtual ~ServiceListener() = default;
	using ListenerInterface::ListenerInterface;

	inline int getType(void) const override { return LT_SERVICE; }

private:
	bool eventAccept(const accept_type& aparam) override
	{
		chif_create_type cparam;
		cparam.fd = aparam.fd;
		cparam.poller = getIoPoller();
		cparam.ssl = aparam.ssl;
		auto pch(new ServiceChannel(cparam));
		return nullptr not_eq pch;
	}
};

int
main( int argc, char* argv[] )
{
	PWINIT();

	do {
		auto poller(IoPoller::s_create());
		if ( not poller )
		{
			fprintf(stderr, "failed to create poller");
			break;
		}

		auto lsnr(new ServiceListener(poller));
		if ( not lsnr )
		{
			fprintf(stderr, "failed to create listener");
			break;
		}

		if ( not lsnr->open(nullptr, SERVICE_PORT) )
		{
			fprintf(stderr, "failed to open port: %s", SERVICE_PORT);
			break;
		}

		while ( poller->dispatch(1000) >= 0 ) continue;
	} while (false);

	return EXIT_FAILURE;
}
