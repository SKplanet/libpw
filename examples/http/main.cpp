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
 * \brief Example for http server.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#define SERVICE_NAME "http"

#include <pw/pwlib.h>
using namespace pw;

class ServiceChannel;
class AdminChannel;

// 에코 채널.
class ServiceChannel final : public HttpServerChannel
{
public:
	// I love C++11!
	virtual ~ServiceChannel() = default;
	using HttpServerChannel::HttpServerChannel;

private:
	StlStringPacket m_recv;

private:
	// 패킷 클래스로 넘겨 로직을 실행.
	void eventReadPacket(const PacketInterface& in, const char*, size_t) override
	{
		auto& req ( dynamic_cast<const HttpRequestPacket&>(in) );
		HttpResponsePacket res;
		res.setResCode(PWRES_CODE_SUCCESS);
		res.m_body.assign(req.m_body, blob_type::CT_POINTER);
		write(res);
	}
};

// 관리자 채널.
class AdminChannel final : public MsgChannel
{
public:
	virtual ~AdminChannel() = default;
	using MsgChannel::MsgChannel;

private:
	void eventReadPacket(const PacketInterface& in, const char*, size_t) override
	{
		auto& pk(dynamic_cast<const MsgPacket&>(in));
		if ( pk.m_code == "EXIT" ) procEXIT(pk);
		else eventError(Error::INVALID_PACKET, 0);
	}

private:
	void procEXIT(const MsgPacket& in);
};

// 싱글 프로세스 리스너 템플릿.
template<typename _ChType, int _Type>
class MyLsnrTmpl final : public ListenerInterface
{
public:
	virtual ~MyLsnrTmpl() = default;
	using ListenerInterface::ListenerInterface;

	inline int getType(void) const override { return _Type; }

private:
	bool eventAccept(const accept_type& aparam) override
	{
		chif_create_type cparam;
		cparam.fd = aparam.fd;
		cparam.poller = getIoPoller();
		cparam.ssl = aparam.ssl;
		auto pch(new _ChType(cparam));
		return nullptr not_eq pch;
	}
};

using ServiceListener = MyLsnrTmpl<ServiceChannel, ListenerInterface::LT_SERVICE>;
using AdminListener = MyLsnrTmpl<AdminChannel, ListenerInterface::LT_ADMIN>;

// 멀티 프로세스 리스너(자식 프로세스용)
class ChildListener final : public ChildListenerInterface
{
public:
	virtual ~ChildListener() = default;
	using ChildListenerInterface::ChildListenerInterface;

private:
	bool eventAccept(const accept_type& aparam) override
	{
		chif_create_type cparam;
		cparam.fd = aparam.fd;
		cparam.poller = getIoPoller();
		cparam.ssl = aparam.ssl;

		ChannelInterface* pch(nullptr);
		switch(aparam.type)
		{
		case LT_SERVICE: pch = new ServiceChannel(cparam);
		case LT_ADMIN: pch = new AdminChannel(cparam);
		default:;
		}

		return nullptr not_eq pch;
	}
};

// 인스턴스 싱글턴 객체.
class MyInstance final : public InstanceInterface
{
public:
	virtual ~MyInstance() = default;
	using InstanceInterface::InstanceInterface;

private:
	// 타이머 이벤트 처리.
	void eventTimer(int id, void* param) override {}

	bool eventConfig(bool isDefault, bool isReload) override
	{
		if ( not m_tls_server.read(getConfig(), "server", "tls") )
		{
			logError(__FILE__, __LINE__, "failed to load tls settings...");
			return false;
		}

		return true;
	}

	// 싱글 프로세스용 리스너 초기화.
	bool eventInitListenerSingle(void) override
	{
		if ( not openListenerSingle<ServiceListener>("http") ) return false;
		if ( not openListenerSingle<ServiceListener>("https") ) return false;
		if ( not openListenerSingle<AdminListener>("admin") ) return false;
		return true;
	}

	// 멀티 프로세스용 리스너 초기화.
	bool eventInitListenerParent(void) override
	{
		if ( not openListenerParent<ListenerInterface::LT_SERVICE_HTTP>("http") ) return false;
		if ( not openListenerParent<ListenerInterface::LT_SERVICE_HTTPS>("https") ) return false;
		if ( not openListenerParent<ListenerInterface::LT_ADMIN>("admin") ) return false;
		return true;
	}

	// 멀티 프로세스용 리스너 초기화.
	bool eventInitListenerChild(void) override
	{
		return this->openListenerChild<ChildListener>(lsnr_names{"http", "https", "admin"});
	}

	SslContext* getListenSslContext(const string& name) const override
	{
		return ( name == "https" ? m_tls_server.ctx : nullptr );
	}

private:
	ssl::context_type m_tls_server;
};

void
AdminChannel::procEXIT(const MsgPacket& in)
{
	MyInstance::s_inst->setFlagRun(false);
}

int
main( int argc, char* argv[] )
{
	PWINIT();
	MyInstance inst(SERVICE_NAME);
	return inst.start(argc, argv);
}
