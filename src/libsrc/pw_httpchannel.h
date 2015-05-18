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
 * \file pw_httpchannel.h
 * \brief Channel for HTTP/1.x.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_packet_if.h"
#include "./pw_channel_if.h"
#include "./pw_jobmanager.h"
#include "./pw_httppacket.h"

#ifndef __PW_HTTPCHANNEL_H__
#define __PW_HTTPCHANNEL_H__

namespace pw {

class HttpClientChannel;
class HttpClientChannelFactoryInterface;

namespace http {

struct query_param_type
{
	//! \brief 출력 파라매터
	struct _out {
		int					err = 0;
		int64_t				timeout = 0;
		HttpClientChannel*	ch = nullptr;
		HttpResponsePacket*	pk = nullptr;
	} out;

	//! \brief 입력 파라매터
	struct _in {
		bool						async = true;
		host_type					host;
		IoPoller*					poller = nullptr;
		Ssl*						ssl = nullptr;
		const HttpRequestPacket*	pk = nullptr;
		JobManager::Job*			job = nullptr;
		int64_t						timeout = 3000LL;
		HttpClientChannelFactoryInterface*	factory = nullptr;
	} in;

	bool setUri(const uri_type& s, SslContext& ctx);
	bool setSsl(const uri_type& s, SslContext& ctx);
	void setHost(const uri_type& uri);
};

//namespace http
};

//! \brief HTTP 채널 인터페이스.
class HttpChannelInterface : public ChannelInterface
{
public:
	inline explicit HttpChannelInterface(const chif_create_type& param) : ChannelInterface(param) {}
	inline explicit HttpChannelInterface() = default;
	inline virtual ~HttpChannelInterface() = default;

	HttpChannelInterface(const HttpChannelInterface&) = delete;
	HttpChannelInterface(HttpChannelInterface&&) = delete;
	HttpChannelInterface& operator = (const HttpChannelInterface&) = delete;
	HttpChannelInterface& operator = (HttpChannelInterface&&) = delete;

	inline size_t getDestinationBodyLength(void) const { return m_dest_bodylen; }
	inline size_t getReceivedBodyLength(void) const { return m_recv_bodylen; }

protected:
	virtual const HttpPacketInterface& getRecvPacket(void) const = 0;
	virtual HttpPacketInterface& getRecvPacket(void) = 0;

protected:
	size_t		m_dest_bodylen = 0; //!< 최종 받을 바디 길이
	size_t		m_recv_bodylen = 0; //!< 현재 받은 바디 길이

protected:
	void eventError(Error type, int err) override;

	//! \brief Request 여부
	virtual bool isRequest(void) const = 0;

	//! \brief 첫줄 읽은 것에 읽었을 때 이벤트 처리
	virtual void eventReadFirstLine(void) {}

	//! \brief 헤더 하나를 읽었을 때 이벤트 처리
	//! \param[in] key 키
	//! \param[in] value 내용
	virtual void eventReadHeader(std::string& key, std::string& value) {}

	//! \brief 바디 섹션을 받았을 때 이벤트 처리
	//! \param[in] event_size 받은 바디 섹션 크기
	virtual void eventReadBody(size_t event_size) {}

	//! \brief 접속 유지 여부.
	virtual bool isKeepAlive(void) const { return true; }

private:
	void eventReadData(size_t len) override final;

friend class HttpClientChannel;
friend class HttpServerChannel;
};

//! \brief 클라이언트 사이드 채널.
//!	요청을 먼저 시작하는 쪽이며, 받은 패킷은 HttpResponsePacket이다.
class HttpClientChannel : public HttpChannelInterface
{
public:
	using query_param_type = pw::http::query_param_type;

public:
	explicit HttpClientChannel(const chif_create_type& param, JobManager::Job* pjob = nullptr);
	explicit HttpClientChannel(JobManager::Job* pjob = nullptr);
	inline virtual ~HttpClientChannel() {}

	//! \brief 쿼리 전송. 무조건 비동기 방식
	static HttpClientChannel* s_query(const host_type& host, const HttpRequestPacket& pk, IoPoller* poller, Ssl* ssl = nullptr, JobManager::Job* pjob = nullptr);

	//! \brief 쿼리 전송.
	static bool s_query(query_param_type& param);

public:
	//! \brief 쿼리 전송.
	bool query(const host_type& host, const HttpRequestPacket& pk, HttpResponsePacket* res_pk = nullptr);

	//! \brief 쿼리를 취소하고, 인스턴스를 삭제한다.
	//!	\warning 메소드 호출 이후 인스턴스를 사용해서는 안 된다.
	void cancelQuery(void);

	inline const HttpResponsePacket& getPacket(void) const { return m_recv; }
	inline const HttpResponsePacket* getHookPacket(void) const { return m_hook_recv; }

protected:
	//! \brief 걸려 있는 잡에 이벤트를 넘긴다.
	bool dispatchJobPacket(void* param = nullptr);

	//! \brief 걸려 있는 잡에 이벤트를 넘긴다.
	bool dispatchJobError(ChannelInterface::Error type, int err);

	virtual void eventFirstLine(http::Version version, int rcode, const std::string& rmesg) {}

	inline const HttpPacketInterface& getRecvPacket(void) const override final { return m_recv; }
	inline HttpPacketInterface& getRecvPacket(void) override final { return m_recv; }

private:

	void eventConnect(void) override;
	inline void eventReadPacket(const PacketInterface& pk, const char* body, size_t bodylen) override { /* do nothing... */ }
	void eventError(Error type, int err) override;

	void hookReadPacket(const PacketInterface& pk, const char* body, size_t bodylen) override;

private:
	HttpResponsePacket	m_recv;
	HttpResponsePacket*	m_hook_recv = nullptr;
	std::string			m_query;

	JobManager*			m_job_man = nullptr;
	const job_key_type	m_job_key = 0;

private:
	bool isRequest(void) const override final { return true; }
	static bool _s_queryAsync(http::query_param_type& param);
	static bool _s_querySync(http::query_param_type& param);
};

//! \brief 서버 사이드 채널.
//!	요청을 받아 응답을 보내는 쪽이며, 받은 패킷은 HttpRequestPacket이다.
class HttpServerChannel : public HttpChannelInterface
{
public:
	inline explicit HttpServerChannel(const chif_create_type& param) : HttpChannelInterface(param) {}
	inline explicit HttpServerChannel() = default;
	inline virtual ~HttpServerChannel() = default;

protected:
	virtual void eventReadFirstLine(http::Method method, const uri_type& uri, http::Version version) {}

	inline const HttpPacketInterface& getRecvPacket(void) const override final { return m_recv; }
	inline HttpPacketInterface& getRecvPacket(void) override final { return m_recv; }

private:
	HttpRequestPacket	m_recv;

private:
	bool isRequest(void) const override final { return false; }
};

//! \brief 클라이언트 채널 팩토리
class HttpClientChannelFactoryInterface
{
public:
	virtual ~HttpClientChannelFactoryInterface() = default;

public:
	virtual HttpClientChannel* create(http::query_param_type& inout) = 0;
};

}; //namespace pw

#endif//!__PW_HTTPCHANNEL_H__

