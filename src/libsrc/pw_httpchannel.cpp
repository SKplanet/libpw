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
 * \file pw_httpchannel.cpp
 * \brief Channel for HTTP/1.x.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_httpchannel.h"
#include "./pw_string.h"
#include "./pw_encode.h"
#include "./pw_compress.h"
#include "./pw_timer.h"
#include "./pw_log.h"
#include "./pw_ssl.h"

namespace pw {

#define ISAGAIN(err) Socket::s_isAgain(err)

inline
static
int64_t
_set_leftTimeout(int64_t in, int64_t term)
{
	if ( (in == 0) or (term >= in) ) return 0;
	return in - term;
}

inline
static
HttpClientChannel*
_createChannel(http::query_param_type& param)
{
	auto& in(param.in);

	if ( in.factory ) return in.factory->create(param);

	chif_create_type cparam(-1, in.poller, in.ssl);
	return new HttpClientChannel(cparam, in.job);
}

namespace http {

bool
query_param_type::setUri ( const uri_type& uri, SslContext& ctx )
{
	int port(uri.getNumericPort());
	const auto is_ssl(isSsl(uri));
	if ( is_ssl )
	{
		auto s(Ssl::s_create(ctx));
		if ( s == nullptr )
		{
			PWLOGLIB("failed to create ssl object");
			return false;
		}

		in.ssl = s;

		if ( not port ) port = 443;
	}
	else
	{
		if ( not port ) port = 80;
	}

	in.host.host = uri.getHost();
	in.host.service = std::to_string(port);

	return true;
}

void
query_param_type::setHost ( const uri_type& uri )
{
	int port(uri.getNumericPort());
	if ( port == 0 )
	{
		port = isSsl(uri.getScheme()) ? 443 : 80;
	}

	in.host.host = uri.getHost();
	in.host.service = std::to_string(port);
}

bool
query_param_type::setSsl ( const uri_type& s, SslContext& ctx )
{
	if ( isSsl(s) )
	{
		auto s(Ssl::s_create(ctx));
		if ( s == nullptr )
		{
			PWLOGLIB("failed to create ssl object");
			return false;
		}

		in.ssl = s;
	}

	return true;
}

};

//------------------------------------------------------------------------------
// querySync hidden class...
// class _pwhttpclientchannel : public HttpClientChannel
// {
// public:
// 	inline explicit _pwhttpclientchannel(bool& ref_dead, const chif_create_type& param, JobManager::Job* pjob) : HttpClientChannel(param, pjob), m_ref_dead(ref_dead) {}
//
// 	virtual ~_pwhttpclientchannel() { m_ref_dead = true; }
//
// private:
// 	bool& m_ref_dead;
// };


//------------------------------------------------------------------------------
// HttpChannelInterface

bool
HttpClientChannel::dispatchJobPacket ( void* param )
{
	if ( m_job_key and m_job_man ) return m_job_man->dispatchPacket(m_job_key, this, getRecvPacket(), param);
	return false;
}

bool
HttpClientChannel::dispatchJobError ( ChannelInterface::Error type, int err )
{
	if ( m_job_key and m_job_man ) return m_job_man->dispatchError(m_job_key, this, type, err );
	return false;
}

void
HttpChannelInterface::eventReadData(size_t len)
{
	//PWTRACE("%s %zu", __PRETTY_FUNCTION__, len);
	HttpPacketInterface& pk(getRecvPacket());

	do {
		switch (m_recv_state)
		{
		case RecvState::START:
		{
			PWTRACE_HEAVY("RecvState::START %p", this);
			pk.clear();
			m_dest_bodylen = size_t(-1);
			m_recv_bodylen = 0;
			setRecvStateFirstLine();
		}// fall to RecvState::FIRST_LINE
		/* no break */
		case RecvState::FIRST_LINE:
		{
			PWTRACE_HEAVY("RecvState::FIRST_LINE %p", this);
			IoBuffer::blob_type b;
			m_rbuf->grabRead(b);

			const char* eol(PWStr::findLine(b.buf, b.size));
			if ( nullptr == eol )
			{
				if ( HttpPacketInterface::MAX_FIRST_LINE_SIZE < b.size )
				{
					PWTRACE_HEAVY("too long first line");
					setRecvStateError();
					break;
				}

				PWTRACE_HEAVY("waiting for first line");
				return;
			}

			size_t cplen(eol - b.buf);
			//do { const std::string tmp(b.buf, cplen); PWTRACE("firstLine:%s", tmp.c_str()); } while (false);
			bool res(pk.setFirstLine(b.buf, cplen));
			m_rbuf->moveRead(cplen+2);

			if ( not res )
			{
				char msgfmt[128];
				snprintf(msgfmt, sizeof(msgfmt), "invalid packet from line: %%.%zus", cplen);
				PWLOGLIB(msgfmt, b.buf);
				setRecvStateError();
				break;
			}

			setRecvStateHeader();
			eventReadFirstLine();
		}// fall to RecvState::HEADER
		/* no break */
		case RecvState::HEADER:
		{
			PWTRACE_HEAVY("RecvState::HEADER");
			IoBuffer::blob_type b;
			bool res(true);
			do {
				m_rbuf->grabRead(b);
				if ( HttpPacketInterface::MAX_HEADER_LINE_SIZE < b.size )
				{
					setRecvStateError();
					break;
				}

				const char* eol(PWStr::findLine(b.buf, b.size));
				if ( nullptr == eol )
				{
					return;
				}

				const size_t cplen(eol - b.buf);
				if ( 0 == cplen )
				{
					m_rbuf->moveRead(cplen+2);
					break;
				}

				Tokenizer tok(b.buf, cplen);
				tok.setStrict(true);

				std::string key, value;
				if ( not tok.getNext(key, ':') )
				{
					char msgfmt[128];
					snprintf(msgfmt, sizeof(msgfmt), "invalid header from line: %%.%zus", cplen);
					PWLOGLIB(msgfmt, b.buf);
					res = false;
					m_rbuf->moveRead(cplen+2);
					break;
				}

				PWStr::trim(key);
				value.assign(tok.getPosition(), tok.getLeftSize());
				PWStr::trim(value);

				//PWTRACE("key:%s value:%s", key.c_str(), value.c_str());

				m_rbuf->moveRead(cplen+2);
				if ( 0 == strcasecmp(key.c_str(), "Content-Length") )
				{
					m_dest_bodylen = strtosize(value.c_str(), nullptr, 10);
				}
				else if ( not (res = pk.setHeader(key, value)) )
				{
					char msgfmt[128];
					snprintf(msgfmt, sizeof(msgfmt), "invalid packet from line: %%.%zus", cplen);
					PWLOGLIB(msgfmt, b.buf);
					break;
				}

				eventReadHeader(key, value);
			} while ( res );

			if ( not res )
			{
				setRecvStateError();
				break;
			}

			if ( size_t(-1) == m_dest_bodylen )
			{
				const auto mt(static_cast<HttpRequestPacket&>(pk).getMethodType());
				if ( (not isRequest())
					and (http::Method::POST not_eq mt)
					and (http::Method::PUT not_eq mt) )
				{
					PWTRACE_HEAVY("not POST");
					m_dest_bodylen = 0;
					pk.m_body.assign("\0", 1, blob_type::CT_POINTER);
					pk.m_body.size = 0;
					setRecvStateDone();
					break;
				}
			}
			else if ( 0 == m_dest_bodylen )
			{
				PWTRACE_HEAVY("no BODY");
				pk.m_body.assign("\0", 1, blob_type::CT_POINTER);
				pk.m_body.size = 0;
				setRecvStateDone();
				break;
			}
			else if ( HttpPacketInterface::MAX_BODY_SIZE < m_dest_bodylen )
			{
				PWLOGLIB("Too large body size: %zu", m_dest_bodylen);
				m_recv_state = RecvState::ERROR;
				break;
			}

			setRecvStateBody();
		}// fall to RecvState::BODY
		/* no break */
		case RecvState::BODY:
		{
			PWTRACE_HEAVY("RecvState::BODY : m_dest_bodylen:%zu m_recv_bodylen:%zu", m_dest_bodylen, m_recv_bodylen);
			auto& body(pk.m_body);
			if ( body.empty() )
			{
				if ( body.allocate(size_t(-1) == m_dest_bodylen ? HttpPacketInterface::DEFAULT_BODY_SIZE : m_dest_bodylen+1) )
				{
					--body.size;
					body[body.size] = 0x00;
				}
				else
				{
					PWLOGLIB("not enough memory");
					setRecvStateError();
					break;
				}
			}

			IoBuffer::blob_type b;
			m_rbuf->grabRead(b);
			size_t cplen(std::min(b.size, (m_dest_bodylen - m_recv_bodylen)));

			if ( cplen > 0 )
			{
				body.append(m_recv_bodylen, b.buf, cplen);
				m_recv_bodylen += cplen;
				m_rbuf->moveRead(cplen);

				eventReadBody(cplen);
			}

			// Content-Length를 명시하지 않았을 경우, 끊길 때까지 읽는다.
			if ( m_recv_bodylen not_eq m_dest_bodylen )
			{
				return;
			}

			setRecvStateDone();
		}// fall to RecvState::DONE
		/* no break */
		case RecvState::DONE:
		{
			//PWTRACE("RecvState::DONE: %p", this);
			if ( size_t(-1) == m_dest_bodylen )
			{
				pk.m_body.append(m_recv_bodylen, "\0", 1);
				pk.m_body.size = m_recv_bodylen;
			}

			PWTRACE_HEAVY("=============> %zu", m_recv_bodylen);
			hookReadPacket(pk, pk.m_body.buf, m_recv_bodylen);

			setRecvStateStart();
			if ( not isKeepAlive() ) setExpired();

			break;
		}
		case RecvState::ERROR:
		{
			//PWTRACE("RecvState::ERROR: %p", this);
			eventError(Error::INVALID_PACKET, 0);
			setRecvStateStart();
			if ( isInstDeleteOrExpired() ) return;
		}
		}//switch
	} while ( true );
}

void
HttpChannelInterface::eventError(Error type, int err)
{
	if ( (Error::READ_CLOSE == type)
		and (RecvState::BODY == m_recv_state)
		and (size_t(-1) == m_dest_bodylen) )
	{
		PWTRACE_HEAVY("RecvState::DONE: %p", this);
		HttpPacketInterface& pk(getRecvPacket());
		pk.m_body.append(m_recv_bodylen, "\0", 1);
		pk.m_body.size = m_recv_bodylen;
		hookReadPacket(pk, pk.m_body.buf, m_recv_bodylen);
		setRecvStateStart();
	}

	ChannelInterface::eventError(type, err);
}

//------------------------------------------------------------------------------
// HttpClientChannel

HttpClientChannel::HttpClientChannel(const chif_create_type& param, JobManager::Job* pjob) : HttpChannelInterface(param), m_job_man(pjob?&(pjob->getManager()):nullptr), m_job_key(pjob?pjob->getKey():0)
{
}

HttpClientChannel::HttpClientChannel(JobManager::Job* pjob) : HttpChannelInterface(), m_job_man(pjob?&(pjob->getManager()):nullptr), m_job_key(pjob?pjob->getKey():0)
{
}

HttpClientChannel*
HttpClientChannel::s_query(const host_type& host, const HttpRequestPacket& pk, IoPoller* poller, Ssl* ssl, JobManager::Job* pjob)
{
	//PWSHOWMETHOD();
	chif_create_type param(-1, poller, ssl);

	HttpClientChannel* ch(new HttpClientChannel(param, pjob));
	if ( nullptr == ch )
	{
		PWTRACE_HEAVY("not enough memory");
		return nullptr;
	}

	if ( not ch->query(host, pk) )
	{
		PWTRACE_HEAVY("failed to query");
		delete ch;
		return nullptr;
	}

	return ch;
}

bool
HttpClientChannel::_s_queryAsync ( http::query_param_type& param )
{
	auto& in(param.in);
	auto& out(param.out);
	errno = 0;

	auto pch(_createChannel(param));
	if ( nullptr == pch )
	{
		PWTRACE_HEAVY("not enough memory");
		errno = out.err = ENOMEM;
		return false;
	}

	if ( not pch->query(in.host, *in.pk, out.pk) )
	{
		PWTRACE_HEAVY("failed to query");
		delete pch;
		out.err = errno;
		return false;
	}

	out.ch = pch;
	out.err = EINPROGRESS;

	return true;
}

bool
HttpClientChannel::_s_querySync ( http::query_param_type& param )
{
	auto& in(param.in);
	auto& out(param.out);
	const int64_t start(Timer::s_getNow());
	errno = 0;

	IoPoller* poller(IoPoller::s_create("auto"));
	if ( nullptr == poller )
	{
		out.timeout = _set_leftTimeout(in.timeout, Timer::s_getNow() - start);
		out.err = errno = ENOSYS;
		return false;
	}

	IoPoller* poller_old(in.poller);
	in.poller = poller;
	auto pch(_createChannel(param));

	do {
		if ( nullptr == pch )
		{
			out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - start);
			out.err = errno = ENOMEM;
			break;
		}

		const auto unique_id(pch->getUniqueName());

		if ( not pch->query(in.host, *in.pk, out.pk) )
		{
			out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - start);
			if ( 0 == errno ) param.out.err = errno = EPIPE;
			else out.err = errno;
			break;
		}

		do {
			poller->dispatch(in.timeout);
			if ( (Timer::s_getNow() - start) > in.timeout )
			{
				out.err = errno = ETIMEDOUT;
				pch->cancelQuery();

				if ( out.pk )
				{
					out.pk->setResCode(ResultCode::GATEWAY_TIMEOUT);
				}

				out.timeout = 0;

				goto ERROR_PROC;
			}
		} while ( ChannelInterface::s_getChannel(unique_id) );

		param.out.err = 0;
		IoPoller::s_release(poller);
		param.out.timeout = _set_leftTimeout(param.in.timeout, Timer::s_getNow() - start);

		param.in.poller = poller_old;
		return true;

	} while (false);

	ERROR_PROC:
	if ( pch ) { delete pch; pch = nullptr; }
	if ( poller ) { IoPoller::s_release(poller); poller = nullptr; }
	param.in.poller = poller_old;

	return false;
}


bool
HttpClientChannel::s_query(http::query_param_type& param)
{
	param.out.ch = nullptr;

	if ( param.in.async )
	{
		return _s_queryAsync(param);
	}

	return _s_querySync(param);
}

// void
// HttpClientChannel::hookReadFirstLine ( void )
// {
// 	this->eventFirstLine(
// 		m_recv.getVersion(),
// 		static_cast<int>(m_recv.getResCode()),
// 		m_recv.getResMessage()
// 	);
// }

void
HttpClientChannel::cancelQuery(void)
{
	m_query.clear();

	if ( isConnSuccess() ) setExpired();
	else releaseInstance();
}

bool
HttpClientChannel::query(const host_type& host, const HttpRequestPacket& pk, HttpResponsePacket* res_pk)
{
	if ( m_hook_recv not_eq res_pk ) m_hook_recv = res_pk;

	if ( not isConnSuccess() )
	{
		if ( not this->connect(host) ) return false;
	}

	if ( isConnSuccess() )
	{
		return write(pk);
	}

	std::string tmp;
	pk.write(tmp);
	m_query.append(tmp);

	return true;
}

void
HttpClientChannel::eventConnect(void)
{
	if ( m_query.empty() ) return;

	write(m_query.c_str(), m_query.size());
	m_query.clear();
}

void
HttpClientChannel::eventError(Error type, int err)
{
	if ( Log::s_getTrace() )
	{
		PWTRACE("type:%s", s_toString(type));
		PWTRACE("m_recv_state:%s", s_toString(m_recv_state));
		PWTRACE("m_recv_bodylen:%zu", m_recv_bodylen);
		PWTRACE("m_dest_bodylen:%zu", m_dest_bodylen);
	}

	if ( (Error::READ_CLOSE == type) and (RecvState::BODY == m_recv_state) and (size_t(-1) == m_dest_bodylen) )
	{
		HttpPacketInterface& pk(getRecvPacket());
		pk.m_body.size = m_recv_bodylen;
		hookReadPacket(pk, pk.m_body.buf, pk.m_body.size);
	}
	else if ( m_job_man and (m_job_key > 0) )
	{
		PWTRACE("recv error!");
		m_job_man->dispatchError(m_job_key, this, type, err);
	}
	else
	{
		PWTRACE("recv nothing");
	}

	m_recv_state = RecvState::START;
	ChannelInterface::eventError(type, err);
}

void
HttpClientChannel::hookReadPacket(const PacketInterface& pk, const char* body, size_t bodylen)
{
	PWTRACE("job_man:%p job_key:%ju", m_job_man, uintmax_t(m_job_key));
	const HttpResponsePacket* rpk(dynamic_cast<const HttpResponsePacket*>(&pk));

	if ( m_hook_recv and rpk and (m_hook_recv not_eq rpk) )
	{
		if ( m_hook_recv->m_body.buf not_eq body )
		{
			m_hook_recv->m_body.assign(body, bodylen+1, blob_type::CT_MALLOC);
			m_hook_recv->m_body.size = bodylen;
		}

		m_hook_recv->m_res_code = rpk->m_res_code;
		m_hook_recv->m_res_mesg = rpk->m_res_mesg;
		m_hook_recv->m_version = rpk->m_version;
		m_hook_recv->m_headers = rpk->m_headers;
	}

	if ( m_job_man and m_job_key )
	{
		PWTRACE("job!");
		m_job_man->dispatchPacket(m_job_key, this, pk);
	}
	else
	{
		PWTRACE("event");
		eventReadPacket(pk, body, bodylen);
	}

	setExpired();
}

//------------------------------------------------------------------------------
// HttpServerCahnnel
// void
// HttpServerChannel::hookReadFirstLine ( void )
// {
// 	this->eventReadFirstLine(
// 		m_recv.getMethodType(),
// 		m_recv.m_uri,
// 		m_recv.getVersion()
// 	);
// }

};//namespace pw
