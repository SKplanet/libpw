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
 * \file pw_listener_if.h
 * \brief Listener classes.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_socket.h"
#include "./pw_sockaddr.h"

#ifndef __PW_LISTENER_IF_H__
#define __PW_LISTENER_IF_H__

namespace pw {

class SslContext;
class Ssl;

//! \brief 기본 리스너 인터페이스
class ListenerInterface : public Socket
{
public:
	//! \brief 리스너 타입
	enum listener_type
	{
		LT_NONE,			//!< Unknown or invalid
		LT_SERVICE,			//!< Service
		LT_SERVICE_SSL,		//!< Service with SSL
		LT_SERVICE_HTTP,	//!< Service with HTTP
		LT_SERVICE_HTTPS,	//!< Service with HTTPS
		LT_ADMIN,			//!< Administrator's
		LT_ADMIN_SSL,		//!< Administrator's with SSL
		LT_APPEND,			//!< Appended
	};

	//! \brief 접속한 클라이언트 타입
	struct accept_type
	{
		mutable ListenerInterface*	lsnr = nullptr;	//!< Caller listener.
		int								fd = -1;		//!< Accepted socket fd.
		listener_type					type = listener_type::LT_NONE;	//!< Listener type.
		SocketAddress					sa;		//!< Socket address.
		mutable Ssl*					ssl = nullptr;	//!< SSL channel.
		mutable void*					append = nullptr;	//!< Appendix.

		explicit inline accept_type() = default;
		explicit inline accept_type(ListenerInterface* _lsnr, int _fd, listener_type _type, const SocketAddress& _sa, Ssl* _ssl, void* _append) : lsnr(_lsnr), fd(_fd), type(_type), sa(_sa), ssl(_ssl), append(_append) {}

		std::ostream& dump(std::ostream& os) const;
	};

public:
	explicit ListenerInterface(IoPoller* poller);
	virtual ~ListenerInterface();

public:
	//! \brief SSL 컨텍스트 설정하기.
	SslContext* setSslContext(SslContext* ctx);

	//! \brief 설정한 SSL 컨텍스트 얻어오기.
	virtual inline SslContext* getSslContext(void) { return m_ssl_ctx; }

	//! \brief 설정한 SSL 컨텍스트 얻어오기.
	virtual inline const SslContext* getSslContext(void) const { return m_ssl_ctx; }

	//! \brief SSL 세션 만들기.
	//! \param[inout] ctx 컨텍스트. 없으면 미리 설정한 컨텍스트를 사용한다.
	virtual Ssl* getNewSsl(SslContext* ctx = nullptr);

	//! \brief 리스너 타입
	inline virtual int getType(void) const { return LT_SERVICE; }

	//! \brief 리스너 개방
	bool open(const char* host, const char* service, int family = PF_INET, int socktype = SOCK_STREAM, int protocol = 0);

	//! \brief 리스너 개방
	bool open(const SocketAddress& sa, int socktype = SOCK_STREAM, int protocol = 0);

	//! \brief 리스너 개방
	bool open(int socktype = SOCK_STREAM, int protocol = 0) { SocketAddress sa; return this->open(sa, socktype, protocol); }

	//! \brief 리스너 폐쇄
	void close(void);

protected:
	//! \brief 새로운 접속을 받아 eventAccept하기 전에 설정할 파라매터 처리.
	//	자식 리스너에서 SSL 생성할 때 사용한다.
	virtual bool eventSetParameters(accept_type& param) { return true; }

	//! \brief 새로운 접속을 받으면 최종적으로 호출.
	virtual bool eventAccept(const accept_type& param) = 0;

protected:
	void eventIo(int fd, int event, bool& del_event);

protected:
	bool			m_auto_async;	//!< 자동 Async모드 소켓 전환

private:
	SslContext*	m_ssl_ctx;

friend class SslDummyChannel;
};

//! \brief 멀티세션 모드에서 부모 프로세스 용 리스너.
//!	인터페이스가 아니므로 그냥 사용해도 무방하다.
class ParentListener : public ListenerInterface
{
public:
	inline int getType(void) const { return m_type; }

public:
	explicit ParentListener(int type, IoPoller* poller);
	virtual ~ParentListener();

protected:
	virtual int getPipeFD(void) const;

protected:
	void eventIo(int fd, int event, bool& del_event);

	//! \warning 딱히 상속 받을 필요는 없다.
	inline bool eventAccept(const accept_type&) { return false; }

private:
	int		m_type;
};

//! \brief 멀티세션 모드에서 자식 프로세스 용 리스너.
//! eventAccept에서 타입에 따라 생성할 채널을 달리해야한다.
class ChildListenerInterface : public ListenerInterface
{
public:
	bool open(int pipe_fd = -1);

public:
	explicit ChildListenerInterface(IoPoller* poller);
	virtual ~ChildListenerInterface();

protected:
	virtual int getPipeFD(void) const;

protected:
	void eventIo(int fd, int event, bool& del_event);

private:
	bool open(const char* host, const char* service, int family = PF_INET, int socktype = SOCK_STREAM, int protocol = 0) = delete;
	bool open(const SocketAddress& sa, int socktype = SOCK_STREAM, int protocol = 0) = delete;
};

};//namespace pw

#endif//!__PW_LISTENER_IF_H__

