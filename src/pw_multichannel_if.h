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
 * \file pw_multichannel_if.h
 * \brief Channel pool management for pw::MsgChannel.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"
#include "./pw_ini.h"
#include "./pw_iopoller.h"
#include "./pw_msgchannel.h"

#ifndef __PW_MULTICHANNEL_IF_H__
#define __PW_MULTICHANNEL_IF_H__

namespace pw {

class MultiChannelFactory;
class MultiChannelInterface;

//! \brief 서비스 채널 풀. 상속하지 말 것.
class MultiChannelPool final
{
public:
	//! \brief 생성 파라매터
	struct create_param_type final
	{
		const Ini*						conf;		//!< 환경설정. 사용자 입력
		std::string						tag;		//!< 서비스 태그. 사용자 입력
		bool							async;		//!< 시작을 Async로 시작할지 여부.  사용자 입력
		mutable chif_create_type		param;		//!< 기본 채널 생성 파라매터
		mutable MultiChannelFactory*	factory;	//!< 팩토리. 사용자 입력
		mutable void*					append;		//!< 추가 파라매터. 사용자 입력
//------------------------------------------------------------------------------
// 이하 자동 변수
		size_t						gname;		//!< 그룹이름
		size_t						index;		//!< 인덱스
		host_type					host;		//!< 접속할 곳
		mutable MultiChannelPool*	pool;		//!< 채널 풀

		explicit create_param_type();
	};

public:
	using ch_type = MultiChannelInterface;

public:
	static MultiChannelPool* s_create(const create_param_type& param);
	static void s_release(MultiChannelPool* pool);

public:
	//! \brief 전체 채널에 브로드캐스트
	//! \param[in] pk 전송할 패킷.
	//! \param[out] pout 선택한 채널 목록.
	//! \return 전송한 채널 개수
	size_t broadcastFull(const PacketInterface& pk, ch_list* pout);

	//! \brief 그룹별 호스트마다 하나씩 캐스트
	//! \param[in] pk 전송할 패킷.
	//! \param[out] pout 선택한 채널 목록.
	//! \return 전송한 채널 개수
	size_t broadcastPerHost(const PacketInterface& pk, ch_list* pout);

	//! \brief 그룹마다 하나씩 캐스트
	//! \param[in] pk 전송할 패킷.
	//! \param[out] pout 선택한 채널 목록.
	//! \return 전송한 채널 개수
	size_t broadcastPerGroup(const PacketInterface& pk, ch_list* pout);

public:
	//! \brief RR 방식으로 다음 채널을 호출한다.
	ch_type* getChannel(void);

	//! \brief 그룹이름을 이용하여 다음 채널을 호출한다.
	ch_type* getChannel(size_t gname);

	//! \brief 재접속 시간 간격을 반환한다.
	inline int64_t getReconnectTime(void) const { return m_reconnect_time; }

	//! \brief 채널을 풀에 넣는다.
	void add(MultiChannelInterface* pch);

	//! \brief 풀에서 채널을 삭제한다.
	//! \warning 채널 객체는 삭제 하지 않으므로 주의하자.
	void remove(MultiChannelInterface* pch);

	//! \brief 컨텐츠 내용을 출력한다.
	std::ostream& dump(std::ostream& os) const;

private:
	explicit MultiChannelPool(const create_param_type& param);
	~MultiChannelPool();

private:
	//! \brief 호스트 하나에 여러 채널을 맺을 때 정보.
	struct chhost_type final
	{
		using ch_cont = std::set<ch_type*>;
		using ch_itr = ch_cont::iterator;
		using ch_citr = ch_cont::const_iterator;

		const host_type	host;
		ch_cont			cont;
		mutable ch_citr	next;

		inline explicit chhost_type(const host_type& _host) : host(_host) {}

		inline bool empty(void) const { return cont.empty(); }

		void add(ch_type* pch);
		void remove(ch_type* pch);
		ch_type* getNext(void) const;
	};

	//! \brief 호스트별 그룹 묶기
	struct chgroup_type final
	{
		using grp_cont = std::map<host_type, chhost_type>;
		using grp_itr = grp_cont::iterator;
		using grp_citr = grp_cont::const_iterator;

		const size_t		gname;
		grp_cont			cont;
		mutable grp_citr	next;

		inline explicit chgroup_type(size_t _gname) : gname(_gname) {}

		inline bool empty(void) const { return cont.empty(); }

		void add(ch_type* pch);
		void remove(ch_type* pch);
		ch_type* getNext(void) const;
	};

	using chpool_cont = std::map<size_t, chgroup_type>;
	using chpool_itr = chpool_cont::iterator;
	using chpool_citr = chpool_cont::const_iterator;

private:
	IoPoller*				m_poller;
	MultiChannelFactory*	m_factory;
	const std::string		m_tag;
	int64_t					m_reconnect_time;

	chpool_cont				m_pool;
	mutable chpool_citr		m_pool_next;

friend class MultiChannelInterface;
};

//! \brief 서비스 채널 인터페이스. 재접속 등을 처리한다.
class MultiChannelInterface : public MsgChannel
{
public:
	enum
	{
		TIMER_RECONNECT_INIT = 19999,		//!< 재접속 시작
		TIMER_RECONNECT_RESPONSE,			//!< 접속 후 응답 기다림
	};

public:
	//! \brief 자신이 속한 그룹 이름을 반환한다.
	inline size_t getGroupName(void) const { return m_gname; }

	//! \brief 풀 내 인덱스를 반환한다.
	inline size_t getIndex(void) const { return m_index; }

	//! \brief 자신이 접속할 호스트를 반환한다.
	inline const host_type& getHost(void) const { return m_host; }

	//! \brief 접속 및 첫 프로토콜 네고를 완료하였는지 확인한다.
	inline bool isConnected(void) const { return m_connected; }

	//! \brief 상대방 이름을 반환한다.
	inline const std::string& getPeerName(void) const { return m_peer_name; }

	//! \brief 인스턴스를 덤프한다.
	std::ostream& dump(std::ostream& os) const;

public:
	explicit MultiChannelInterface(const MultiChannelPool::create_param_type& param);
	virtual ~MultiChannelInterface();

protected:
	//! \brief 접속 후 보낼 최초 패킷을 반환한다.
	//! \param[out] pk 보낼 패킷
	//! \param[out] flag_send 패킷을 보낼지 여부
	//! \param[out] flag_wait 서버로부터 패킷을 기다릴지 여부
	//! \return false일 경우, 접속 실패로 간주하고 재접속을 시도한다.
	virtual bool getHelloPacket(MsgPacket& pk, bool& flag_send, bool& flag_wait) = 0;

	//! \brief 접속 후 받은 최초 패킷을 검사한다.
	//! \param[out] peer_name 상대 서버 이름이며, 패킷으로부터 추출한다.
	//! \param[in] pk 최초 패킷.
	//! \return false를 반환하면 채널을 정리한다.
	virtual bool checkHelloPacket(std::string& peer_name, const MsgPacket& pk) = 0;

	//! \brief 접속이 완료 되었을 때, 호출하는 이벤트 메소드.
	virtual void eventConnected(void) = 0;

	//! \brief 접속이 끊겼을 때, 호출하는 이벤트 메소드.
	virtual void eventDisconnected(void) = 0;

	//! \brief 핑 타임아웃 시, 호출하는 이벤트 메소드.
	void eventPingTimeout(void);

	//! \brief 동기적으로 접속을 시도한다. 프로세스는 반환 전까지 멈춘다.
	//!	타임아웃은 ::alram(2)으로 구현되어 있으며, 5초이다.
	//! 접속을 하면 자동으로 getHelloPacket를 호출하여 최초 패킷을 전송한다.
	bool connectSync(void);

	//! \brief 재접속을 시작한다.
	void reconnect(void);

	//! \brief 강제로 접속을 종료한다.
	void disconnect(void);

protected:
	void setConnected(void);
	void setDisconnected(void);

protected:
	void eventTimer(int id, void* param);

private:
	void eventConnect(void);
	void eventError(Error type, int err);

private:
	void hookReadPacket(const MsgPacket& pk, const char* body, size_t blen);

private:
	// 채널풀 관련 정보
	MultiChannelPool*	m_ch_pool;		//!< 채널풀
	const size_t		m_gname;		//!< 그룹 이름
	const size_t		m_index;		//!< 인덱스
	const host_type		m_host;			//!< 접속할 곳
	bool				m_connected;	//!< 접속 완료 여부
	std::string			m_peer_name;	//!< 상대방 서버 이름

friend class MultiChannelPool;
};

//! \brief 서비스 채널 팩토리
class MultiChannelFactory
{
public:
	//inline explicit MultiChannelFactory() {}
	virtual ~MultiChannelFactory();

	virtual MultiChannelInterface* create(MultiChannelPool::create_param_type& param) = 0;
	virtual void release(MultiChannelInterface* pch);
};

};//namespace pw

#endif//!__PW_MULTICHANNEL_IF_H__
