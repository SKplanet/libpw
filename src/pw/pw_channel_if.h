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
 * \file pw_channel_if.h
 * \brief Channel interface.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_socket.h"
#include "./pw_iobuffer.h"
#include "./pw_packet_if.h"
#include "./pw_timer.h"

#ifndef __PW_CHANNEL_IF_H__
#define __PW_CHANNEL_IF_H__

namespace pw {

class Ssl;
class SslContext;
class IoPoller;

using ch_name_type = uint32_t;

//! \brief 생성 파라매터.
struct chif_create_type final
{
	int						fd{-1};			//!< File descriptor
	mutable IoPoller*	poller{nullptr};		//!< 폴러
	mutable Ssl*			ssl{nullptr};		//!< Ssl channel
	size_t					bufsize{IoBuffer::DEFAULT_SIZE};	//!< 버퍼 크기

	mutable void*			append{nullptr};		//!< 추가 설정

	inline chif_create_type() = default;
	inline chif_create_type(int _fd, IoPoller* _poller, Ssl* _ssl = nullptr, size_t _bufsize = IoBuffer::DEFAULT_SIZE, void* _append = nullptr) : fd(_fd), poller(_poller), ssl(_ssl), bufsize(_bufsize), append(_append) {}
	chif_create_type(int _fd, IoPoller* _poller, const SslContext* _ctx, size_t _bufsize = IoBuffer::DEFAULT_SIZE, void* _append = nullptr);

	std::ostream& dump(std::ostream& os) const;
};

//! \brief 매드 채널 인터페이스. 모든 채널은 인터페이스를 상속하여 만든다.
class ChannelInterface : public Socket
{
public:
	//! \brief 인스턴스 상태.
	enum class InstanceState
	{
		NORMAL,		//!< 일반상태
		DELETE,		//!< 인스턴스 삭제
		EXPIRED,	//!< 곧 지워질 인스턴스. OUT이벤트 시 처리한다.
	};

	//! \brief 접속 형태. connect 메소드 사용시에만 쓰임.
	enum class ConnectState
	{
		NONE,	//!< 접속하지 않음.
		SEND,	//!< SYN을 보냈고, 쓰기 이벤트를 기다리는 중
		FAIL,	//!< 접속 실패
		SUCC,	//!< 접속 성공
		SSL_HANDSHAKING,	//!< SSL 네고 중
		EX_HANDSHAKING,	//!< 추가 프로토콜 네고 중
	};

	//! \brief 검사 형태.
	enum class Check : int
	{
		NONE = 0,		//!< 검사하지 않음
		READ = 0x001,	//!< 읽기버퍼만 검사
		WRITE = 0x002,	//!< 쓰기버퍼만 검사
		BOTH = 0x003,	//!< 모두 검사
	};

	//! \brief 오류 형태. eventError에서 사용
	enum class Error
	{
		NORMAL,		//!< 소켓 오류
		CONNECT,	//!< 커넥트 오류
		READ_CLOSE,	//!< read에서 0 리턴
		READ,		//!< read에서 -1 리턴
		WRITE,		//!< write에서 -1 리턴
		INVALID_PACKET,	//!< 해석단에서 패킷 오류
		SSL_HANDSHAKING,//!< SSL 핸드쉐이킹 오류
		EX_HANDSHAKING,	//!< 추가 핸드쉐이킹 오류
	};

	//! \brief 패킷을 읽은 상태
	enum class RecvState
	{
		START,		//!< 최초 상태
		FIRST_LINE,	//!< 첫줄
		HEADER,		//!< 헤더 읽는 중
		BODY,		//!< 바디 읽는 중
		DONE,		//!< 다 읽음
		ERROR,		//!< 패킷 오류
	};

	enum
	{
		SOCKBUF_SIZE_CHECK = 1024*500,	//!< 소켓버퍼 검사 시 임계값
	};

public:
	explicit ChannelInterface(const chif_create_type& param);
	explicit ChannelInterface();
	virtual ~ChannelInterface();

	ChannelInterface(int, IoPoller*) = delete;
	ChannelInterface& operator = (const ChannelInterface&) = delete;
	ChannelInterface& operator = (ChannelInterface&&) = delete;

public:
	//! \brief 전체 채널 개수를 반환한다.
	static size_t s_getCount(void);

	//! \brief 디버그를 위한 채널 내용을 덤프한다.
	virtual std::ostream& dump(std::ostream& os) const;

	//! \brief 에러 종류에 해당하는 문자열을 반환한다.
	static const char* s_toString(Error e);
	static const char* s_toString(InstanceState i);
	static const char* s_toString(ConnectState c);
	static const char* s_toString(Check c);
	static const char* s_toString(RecvState r);

	//! \brief 유일한 이름으로 채널을 얻는다.
	static ChannelInterface* s_getChannel(ch_name_type unique_name);

	//! \brief 유일한 이름을 얻는다.
	inline ch_name_type getUniqueName(void) const { return m_unique_name; }

	//! \brief 이벤트 받았을 때 디스패치할 개수.
	inline virtual size_t getEventDispatchCount(void) const { return 1; }

	//! \brief 버퍼를 확인한다.
	inline bool checkBuffer(void) const { return ( m_rbuf and m_wbuf ); }

	//! \brief 버퍼를 얻는다.
	inline IoBuffer* getReadBuffer(void) { return m_rbuf; }
	inline const IoBuffer* getReadBuffer(void) const { return m_rbuf; }

	//! \brief 버퍼를 얻는다.
	inline IoBuffer* getWriteBuffer(void) { return m_wbuf; }
	inline const IoBuffer* getWriteBuffer(void) const { return m_wbuf; }

	//! \brief 인스턴스 삭제 상태인지 확인한다.
	inline bool isInstDelete(void) const { return InstanceState::DELETE == m_inst_state; }

	//! \brief 인스턴스 만료 상태인지 확인한다.
	inline bool isInstExpired(void) const { return InstanceState::EXPIRED == m_inst_state; }

	//! \brief 인스턴스가 삭제 또는 만료 상태인지 확인한다.
	inline bool isInstDeleteOrExpired(void) const { return (m_inst_state == InstanceState::EXPIRED) or (m_inst_state == InstanceState::DELETE); }

	inline bool isConnNone(void) const { return ConnectState::NONE == m_conn_state; }
	inline void setConnNone(void) { m_conn_state = ConnectState::NONE; }

	inline bool isConnFail(void) const { return ConnectState::FAIL == m_conn_state; }
	inline void setConnFail(void) { m_conn_state = ConnectState::FAIL; }

	//! \brief 커넥트가 SSL 핸드쉐이킹 상태인지 확인한다.
	inline bool isConnSslHandShaking(void) const { return ConnectState::SSL_HANDSHAKING == m_conn_state; }
	inline void setConnSslHandShaking(void) { m_conn_state = ConnectState::SSL_HANDSHAKING; }

	//! \brief 커넥트가 추가 네고 상태인지 확인한다.
	inline bool isConnExHandShaking(void) const { return ConnectState::EX_HANDSHAKING == m_conn_state; }
	inline void setConnExHandShaking(void) { m_conn_state = ConnectState::EX_HANDSHAKING; }

	//! \brief 추가 네고가 있는 채널인지 확인한다.
	virtual inline bool isExHandShakingChannel(void) const { return false; }

	//! \brief 커넥트가 요청 상태인지 확인한다.
	inline bool isConnSend(void) const { return ConnectState::SEND == m_conn_state; }
	inline void setConnSend(void) { m_conn_state = ConnectState::SEND; }

	//! \brief 커넥트가 완료 상태인지 확인한다.
	inline bool isConnSuccess(void) const { return ConnectState::SUCC == m_conn_state; }
	inline void setConnSuccess(void) { m_conn_state = ConnectState::SUCC; }

	//! \brief 버퍼검사가 쓰기 상태인지 확인한다.
	inline bool isCheckWrite(void) const { return (static_cast<int>(Check::WRITE) bitand static_cast<int>(m_check_type)); }
	inline void setCheckWrite(void) { m_check_type = Check::WRITE; }
	inline void setCheckRead(void) { m_check_type = Check::READ; }
	inline void setCheckBoth(void) { m_check_type = Check::BOTH; }

public:
	virtual bool write(const PacketInterface& pk);
	virtual bool write(const char* buf, size_t blen);

	//! \brief 접속을 시도한다.
	//! \warning 비동기 접속일 경우, 접속 시도 중일 때는 true를 반환하며, isConnSuccess()메소드로 접속 여부를 확인할 수 있다.
	inline bool connect(const char* host, const char* service, int family = PF_UNSPEC, bool async = true) { return this->procConnect(host, service, family, async); }

	//! \brief 접속을 시도한다.
	//! \warning 비동기 접속일 경우, 접속 시도 중일 때는 true를 반환하며, isConnSuccess()메소드로 접속 여부를 확인할 수 있다.
	inline bool connect(const host_type& host, int family = PF_UNSPEC, bool async = true) { return this->procConnect(host.host.c_str(), host.service.c_str(), family, async); }

	//! \brief 라인을 가져온다. 라인이 끝나지 않았을 경우, 무한히 기다린다.
	bool getLineSync(std::string& out, size_t limit = size_t(-1));

	//! \brief 데이터를 가져온다. 라인이 끝나지 않았을 경우, 무한히 기다린다.
	bool getDataSync(std::string& out, size_t size);

	//! \brief 데이터를 가져온다. 라인이 끝나지 않았을 경우, 무한히 기다린다.
	bool getDataSync(void* out, size_t size);

	bool acceptSsl(Error* err = nullptr);
	bool connectSsl(Error* err = nullptr);
	bool handshakeSsl(Error* err = nullptr);

	bool acceptEx(void);
	bool connectEx(void);
	bool handshakeEx(void);

	virtual void setExpired(void);
	virtual void setRelease(void);

	//! \brief 인스턴스를 정리한다. 폴러에서 이벤트가 빠지며, 인스턴스와 커넥션 상태를 초기화하고, 각 버퍼는 비운다.
	void clearInstance(void);

	//! \brief 인스턴스를 해제한다. 소켓을 정리하고, this->destroy()를 호출한다. 기본은 delete this이다.
	void releaseInstance(void);
	virtual void destroy(void);

	void close(void) override;

protected:
	//--------------------------------------------------------------------------
	// High level events

	//! \brief 접속을 완료하면 호출한다.
	virtual void eventConnect(void);

	//! \brief 읽기버퍼에 이벤트가 발생하면 호출한다.
	virtual void eventReadData(size_t len) = 0;

	//! \brief 쓰기버퍼에 이벤트가 발생하면 호출한다.
	inline virtual void eventWriteData(size_t len) { /* do nothing */ }

	//! \brief 버퍼가 넘칠 경우 호출한다. 보통은 로그를 찍기 위한 용도.
	virtual void eventOverflow(int event, size_t nowlen, size_t maxlen);

	//! \brief 온전한 패킷 하나를 받았을 경우 호출한다.
	virtual void eventReadPacket(const PacketInterface& pk, const char* body, size_t blen) = 0;

protected:
	//--------------------------------------------------------------------------
	// Low level events
	// 아래 이벤트를 직접 상속하지 않도록 한다.

	//! \brief 읽기 가능 이벤트 처리.
	//! \warning 직접 상속해서 변경하지 마시오.
	virtual void eventRead(int event);

	//! \brief 쓰기 가능 이벤트 처리.
	//! \warning 직접 상속해서 변경하지 마시오.
	virtual void eventWrite(int event);

	//! \brief 오류 발생 시 처리.
	virtual void eventError(Error err, int my_errno);

	//! \brief 비동기 접속을 처리하기 위한 과정. 특히 SSL채널일 경우 처리.
	virtual void eventConnecting(void);

	//! \brief eventReadPacket으로 보내기 전에 로직 구현을 위한 후커로 사용.
	//! \warning 어플리케이션단에서 오버라이드 하지 않는다.
	inline virtual void hookReadPacket(const PacketInterface& pk, const char* body, size_t blen)
	{
		this->eventReadPacket(pk, body, blen);
	}

	//! \brief eventConnect로 보내기 전에 로직 구현을 위한 후커로 사용.
	//! \warning 어플리케이션단에서 오버라이드 하지 않는다.
	inline virtual void hookConnect(void)
	{
		this->eventConnect();
	}

	inline void setRecvState(RecvState v) { m_recv_state = v; }
	inline void setRecvStateStart(void) { m_recv_state = RecvState::START; }
	inline void setRecvStateFirstLine(void) { m_recv_state = RecvState::FIRST_LINE; }
	inline void setRecvStateHeader(void) { m_recv_state = RecvState::HEADER; }
	inline void setRecvStateBody(void) { m_recv_state = RecvState::BODY; }
	inline void setRecvStateDone(void) { m_recv_state = RecvState::DONE; }
	inline void setRecvStateError(void) { m_recv_state = RecvState::ERROR; }

	virtual bool procAcceptEx(int& revent) { revent = 0; return false; }
	virtual bool procConnectEx(int& revent) { revent = 0; return false; }
	virtual bool procHandshakeEx(int& revent) { revent = 0; return false; }

	virtual bool procConnect(const char* host, const char* service, int family, bool async);

	//! \brief IoPoller::Client에서 상속
	void eventIo(int fd, int event, bool& del_event) override;

protected:
	Ssl*				m_ssl;	//!< Ssl session
	IoBuffer*			m_rbuf;	//!< Read buffer
	IoBuffer*			m_wbuf;	//!< Write buffer

protected:
	InstanceState	m_inst_state = InstanceState::NORMAL;	//!< Instance state
	ConnectState	m_conn_state = ConnectState::NONE;		//!< Connection state
	RecvState		m_recv_state = RecvState::START;		//!< Recv state
	Check			m_check_type = Check::NONE;				//!< Overflow check type

private:
	const ch_name_type		m_unique_name;	//!< Channel unique name
};

template<typename _Type>
class ChannelMapTemplate final
{
public:
	using value_type = _Type;
	using pointer_type = _Type*;

private:
	std::atomic<ch_name_type> m_name;
	mutable std::mutex m_lock;
	std::unordered_map<ch_name_type, pointer_type> m_cont;

public:
	inline ch_name_type insert(pointer_type p)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		ch_name_type tmp(m_name+1);
		while ( not m_cont.insert({tmp, p}).second )
		{
			++tmp;
			if ( tmp == 0 )  ++tmp;
		}

		return (m_name = tmp);
	}

	inline void erase(ch_name_type name)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_cont.erase(name);
	}

	inline void erase(pointer_type p)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		m_cont.erase(p->getUniqueName());
	}

	inline size_t size(void) const
	{
		std::unique_lock<std::mutex> lock(m_lock);
		return m_cont.size();
	}

	inline pointer_type get(ch_name_type name)
	{
		std::unique_lock<std::mutex> lock(m_lock);
		auto ib(m_cont.find(name));
		return ( ib == m_cont.end() ) ? nullptr : ib->second;
	}
};

//! \brief 자주 쓰이는 매드 채널 셋
using ch_set = std::set<ChannelInterface*>;

//! \brief 자주 쓰이는 매드 채널 셋
using ch_uset = std::unordered_set<ChannelInterface*>;

//! \brief 자주 쓰이는 매드 채널 목록
using ch_list = std::list<ChannelInterface*>;

//! \brief 핑 관련 기본 액션 인터페이스
class ChannelPingInterface
{
public:
	virtual ~ChannelPingInterface() = default;

public:
	inline void updateLastReadTime(void) { this->m_last_read = Timer::s_getNow(); }
	inline void updateLastReadTime(int64_t msec) { this->m_last_read = msec; }
	inline int64_t getLastReadPacketTime(void) const { return this->m_last_read; }
	inline intmax_t getDiffFromLastRead(void) const { return static_cast<intmax_t>(Timer::s_getNow() - this->m_last_read); }
	inline intmax_t getDiffFromLastRead(uint64_t msec) const { return static_cast<intmax_t>(msec - this->m_last_read); }

protected:
	//! \brief 유휴 시간을 검사한다.
	//!	false를 반환할 경우 eventPingTimeout을 호출한다.
	virtual bool checkPingTimeout(void) = 0;

	//! \brief 유휴 시간이 InstanceInterface::m_timeout.ping을 넘었을 경우,
	//!	호출된다.
	virtual void eventPingTimeout(void) = 0;

protected:
	int64_t			m_last_read = Timer::s_getNow();	//!< 마지막 패킷 받은 시간. 단위: 밀리초
};

}; //namespace pw

#endif//!__PW_CHANNEL_IF_H__
