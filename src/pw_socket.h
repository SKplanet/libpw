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
 * \file pw_socket.h
 * \brief Default socket class.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_iopoller.h"

#ifndef __PW_SOCKET_H__
#define __PW_SOCKET_H__

namespace pw {

//! \brief 기본 소켓 클래스
class Socket : public IoPoller::Event
{
public:
	//! \brief 셧다운 형태
	using Shutdown = enum class Shutdown : int
	{
		READ = 0,		//!< 읽기만 차단
		WRITE = 1,		//!< 쓰기만 차단
		BOTH = 2,		//!< 모두 차단
	};

	//! \brief 접속 파라매터 타입
	using  connect_param_type = struct connect_param_type final
	{
		// OUT
		struct _out {
			int fd = -1;				//!< Socket FD
			int err = 0;			//!< Error#. Zero is success.
			int64_t timeout = 0;	//!< Left time.
		} out;

		// IN
		struct _in {
			bool			async = true;	//!< Use asynchronos-socket
			int64_t			timeout = 3000LL;//!< Timeout millisecond. Default is 3000ms.
			host_type	host;	//!< Host
			int				family = PF_UNSPEC;	//!< Family. Default is PF_UNSPEC.
		} in;
	};

	//! \brief 읽고 쓰기 파라매터 타입
	using io_param_type = struct io_param_type final
	{
		// OUT
		struct _out {
			int 	err = 0;
			int64_t timeout = 0;
			size_t	size = 0;
		} out;

		// IN
		struct _in {
			int			fd = -1;
			bool		async = true;
			int64_t		timeout = 3000LL;
			const char* buffer = nullptr;
			size_t		size = 0;
			int			flag = 0;
		} in;
	};

public:
	// Socket utilities

	//! \brief 소켓을 넌블록킹 모드로 바꾸기
	static bool s_setNonBlocking(int fd);

	//! \brief 소켓을 넌블록킹 모드로 바꾸기
	static bool s_setNonBlocking(int fd, bool block);

	static bool s_isNonBlocking(int fd);

	//! \brief Nagle 알고리즘 제어
	static bool s_setNoDelay(int fd, bool nodelay = true);

	//! \brief 보내기 버퍼 크기 바꾸기
	static bool s_setSendBufferSize(int fd, size_t blen);

	//! \brief 받기 버퍼 크기 바꾸기
	static bool s_setReceiveBufferSize(int fd, size_t blen);

	//! \brief 접속유지 패킷 사용
	static bool s_setKeepAlive(int fd, bool keepalive = true);

	//! \brief 주소 재사용
	static bool s_setReuseAddress(int fd, bool reuse = true);

	//! \brief ::sendmsg 구현.
	//!	소켓으로 다른 프로세스에 FD 넘기기.
	static ssize_t s_sendMessage(int pipe_fd, int target_fd, const char* buf, size_t blen);

	//! \brief ::recvmsg 구현.
	//!	소켓으로 다른 프로세스로부터 FD 받기.
	static ssize_t s_receiveMessage(int pipe_fd, int& target_fd, char* buf, size_t blen);

	//! \brief 커넥트 구현.
	//! \return 접속을 완료하면, true를 반환한다. Async 접속 연결 진행 중이면, false를 반환하고, errno를 EINPROGRESS로 설정한다.
	static bool s_connect(int& sockfd, const char* host, const char* service, int family = PF_UNSPEC, bool async = false);

	inline static bool s_connect(int& sockfd, const host_type& host, int family = PF_UNSPEC, bool async = false) { return s_connect(sockfd, host.host.c_str(), host.service.c_str(), family, async); }

	//! \brief 타임아웃을 적용한 커넥트 구현.
	static bool s_connectSync(int& sockfd, const char* host, const char* service, int family = PF_UNSPEC, size_t to_msec = 3000UL);

	inline static bool s_connectSync(int& sockfd, const host_type& host, int family = PF_UNSPEC, size_t to_msec = 3000UL) { return s_connectSync(sockfd, host.host.c_str(), host.service.c_str(), family, to_msec); }

	static bool s_connect(connect_param_type& param);

	static bool s_isConnected(int fd, int* err = nullptr);

	inline static bool s_isAgain(int en) { return ( (en == EINPROGRESS) or (en == EAGAIN) or (en == EINTR) ); }
	inline static bool s_isAgain(void) { return ( (errno == EINPROGRESS) or (errno == EAGAIN) or (errno == EINTR) ); }

	static bool s_receive(io_param_type& param);
	static bool s_send(io_param_type& param);

public:
	explicit Socket(int fd, IoPoller* poller);
	inline Socket() : Socket(-1, nullptr) {}
	Socket(const Socket&) = delete;
	Socket(Socket&& s) = delete;
	virtual ~Socket();

	Socket& operator = (const Socket&) = delete;
	Socket& operator = (Socket&&) = delete;

public:
	inline bool setNonBlocking(void) { return s_setNonBlocking(m_fd); }
	inline bool isConnected(int* err = nullptr) { return s_isConnected(m_fd, err); }

	virtual void close(void);
	bool shutdown(Shutdown type = Shutdown::BOTH);

	inline int getHandle(void) const { return m_fd; }
	inline IoPoller* getIoPoller(void) { return m_poller; }
	inline const IoPoller* getIoPoller(void) const { return m_poller; }

public:
	inline bool addToIoPoller(int mask) { if ( m_poller and m_fd >= 0 ) return m_poller->add(m_fd, this, mask); return false; }
	inline bool removeFromIoPoller(void) { if ( m_poller and m_fd >= 0 ) return m_poller->remove(m_fd); return false; }
	inline bool setIoPollerMask(int mask) { if ( m_poller and m_fd >= 0 ) return m_poller->setMask(m_fd, mask); return false; }
	inline bool orIoPollerMask(int mask) { if ( m_poller and m_fd >= 0 ) return m_poller->orMask(m_fd, mask); return false; }
	inline bool andIoPollerMask(int mask) { if ( m_poller and m_fd >= 0 ) return m_poller->andMask(m_fd, mask); return false; }

protected:
	int			m_fd = -1;		//!< 소켓 FD
	IoPoller*	m_poller = nullptr;	//!< 폴러
};

};//namespace pw

#endif//!__PW_SOCKET_H__

