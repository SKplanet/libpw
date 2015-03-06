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
 * \file pw_multichannel_if.cpp
 * \brief Channel pool management for pw::MsgChannel.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_multichannel_if.h"
#include "./pw_string.h"
#include "./pw_instance_if.h"
#include <signal.h>

namespace pw {

static
void
sigALRM(int)
{
	// do nothing
	// interrupt for system calls
}

//------------------------------------------------------------------------------
// Multi Channel Pool

MultiChannelPool::create_param_type::create_param_type()
{
	InstanceInterface* pinst(InstanceInterface::s_inst);
	PWTRACE("pinst: %p", pinst);
	conf = pinst ? &(pinst->m_config.conf) : nullptr;
	async = true;
	param.poller = pinst ? pinst->getPoller() : nullptr;
	factory = nullptr;
	append = nullptr;

	gname = size_t(-1);
	index = size_t(-1);
	pool = nullptr;
}

// chhost_type...
MultiChannelPool::ch_type*
MultiChannelPool::chhost_type::getNext(void) const
{
	if ( cont.empty() ) return nullptr;
	ch_type* ret (nullptr);
	size_t count(0), total(cont.size());
	while ( count < total )
	{
		if ( (ret = const_cast<ch_type*>(*next))->isConnected() )
		{
			if ( cont.end() == ++next ) next = cont.begin();
			return ret;
		}

		++count;
		if ( cont.end() == ++next ) next = cont.begin();
	}

	return nullptr;
}

void
MultiChannelPool::chhost_type::add(ch_type* pch)
{
	if ( cont.empty() ) next = (cont.insert(pch)).first;
	else cont.insert(pch);
}

void
MultiChannelPool::chhost_type::remove(ch_type* pch)
{
	ch_itr ib(cont.find(pch));
	if ( ib == next ) getNext();
	cont.erase(ib);
}

// chgroup_type...
MultiChannelPool::ch_type*
MultiChannelPool::chgroup_type::getNext(void) const
{
	if ( cont.empty() ) return nullptr;

	ch_type* ret(nullptr);
	size_t count(0), total(cont.size());

	while ( (nullptr == (ret = next->second.getNext())) and (count < total) )
	{
		++count;
		if ( cont.end() == ++next ) next = cont.begin();
	}

	if ( nullptr not_eq ret )
	{
		if ( cont.end() == ++next ) next = cont.begin();
	}

	return ret;
}

void
MultiChannelPool::chgroup_type::add(ch_type* pch)
{
	const host_type& host(pch->getHost());
	auto ib(cont.find(host));

	bool bFirst(cont.empty());
	if ( cont.end() == ib )
	{
		ib = cont.insert(grp_cont::value_type(host, chhost_type(host))).first;
	}

	ib->second.add(pch);
	if ( bFirst ) next = ib;
}

void
MultiChannelPool::chgroup_type::remove(ch_type* pch)
{
	const host_type& host(pch->getHost());
	grp_itr ib(cont.find(host));

	if ( cont.end() == ib ) return;

	if ( ib == next ) getNext();

	chhost_type& cht(ib->second);
	cht.remove(pch);
	if ( cht.empty() ) cont.erase(ib);
}

void
MultiChannelPool::s_release(MultiChannelPool* pool)
{
	if ( nullptr == pool ) return;

	chpool_itr ib_pool(pool->m_pool.begin());
	chgroup_type::grp_itr ib_group;
	chhost_type::ch_itr ib_host;
	ch_type* pch(nullptr);

	while ( ib_pool not_eq pool->m_pool.end() )
	{
		chgroup_type& group(ib_pool->second);
		ib_group = group.cont.begin();
		while ( ib_group not_eq group.cont.end() )
		{
			chhost_type& host(ib_group->second);
			ib_host = host.cont.begin();
			while ( ib_host not_eq host.cont.end() )
			{
				pch = *ib_host;
				++ib_host;

				pch->setDisconnected();
				delete pch;
			}// host loop

			++ib_group;
		}// group loop

		++ib_pool;
	}// pool loop

	delete pool;
}

MultiChannelPool*
MultiChannelPool::s_create(const create_param_type& _param)
{
	MultiChannelPool* pool(nullptr);

	std::list<MultiChannelInterface*> tmp_cont;
	ch_type* pch(nullptr);

	do {
		create_param_type param(_param);
		if ( nullptr == (pool = new MultiChannelPool(param)) )
		{
			PWLOGLIB("not enough memory");
			break;
		}

		const Ini& conf(*param.conf);
		MultiChannelFactory& factory(*param.factory);
		host_type& host(param.host);

		// 섹션 찾기
		char secname[64];
		snprintf(secname, sizeof(secname), "multi_%s", param.tag.c_str());
		Ini::sec_citr sec(conf.cfind(secname));
		if ( sec == conf.end() )
		{
			PWLOGLIB("no multi section: tag:%s secname:%s", param.tag.c_str(), secname);
			break;
		}

		// 환경 읽기

		// 재접속 텀
		pool->m_reconnect_time = conf.getInteger("reconnect.time", sec, 1000LL);

		// 그룹개수
		const size_t count(conf.getInteger("count", sec, 0));

		// 호스트별 채널 개수
		const size_t count_dup(conf.getInteger("count.dup", sec, 0));

		if ( 0 == count )
		{
			PWTRACE("no multi channel: tag:%s secname:%s", param.tag.c_str(), secname);
		}

		// 채널 생성
		char itemname[64];

		param.pool = pool;

		string_list	hosts;
		std::string		hline;

		// 객체 생성 루틴
		// chXXX.host=HOST:SERVICE HOST:SERVICE HOST:SERVICE
		bool bForRes(true);

		for ( size_t gname(0), index(0); (gname < count) and bForRes ; gname++ )
		{
			snprintf(itemname, sizeof(itemname), "ch%zu.host", gname);
			//PWTRACE("itemname:%s secname:%s", itemname, secname);
			hline.clear();
			conf.getString2(hline, itemname, sec);
			PWStr::split(hosts, hline);

			param.gname = gname;

			for ( auto ib(hosts.begin()), ie(hosts.end()); (ib not_eq ie) and bForRes; ib ++ )
			{
				param.host = *ib;
				PWTRACE("itemname:%s secname:%s gname:%zu host:%s:%s", itemname, secname, gname, host.host.c_str(), host.service.c_str());

				for ( size_t dup(0); (dup < count_dup) and bForRes; dup++ )
				{
					param.index = index;
					++index;

					if ( nullptr == (pch = factory.create(param)) )
					{
						PWLOGLIB("failed to create multichannel: tag:%s gname:%zu host:%s:%s factory-type:%s", param.tag.c_str(), gname, host.host.c_str(), host.service.c_str(), typeid(factory).name());
						bForRes = false;
						break;
					}

					tmp_cont.push_back(pch);
					pool->add(pch);
				}// for dup
			}// for host
		}// for group

		if ( not bForRes ) break;
		bForRes = true;

		// 객체 초기화 루틴
		for (auto ib(tmp_cont.begin()), ie(tmp_cont.end()); ib != ie; ib++ )
		{
			pch = *ib;

			PWTRACE("connect: ch:%p async:%d host:%s:%s", pch, int(param.async), pch->m_host.host.c_str(), pch->m_host.service.c_str());
			if ( param.async ) pch->reconnect();
			else if ( not pch->connectSync() )
			{
				PWLOGLIB("failed to connect: %s:%s secname:%s", pch->m_host.host.c_str(), pch->m_host.service.c_str(), secname);
				bForRes = false;
				break;
			}
		}

		if ( not bForRes ) break;

		//pool->dump(std::cerr);
		return pool;
	} while (false);

	for ( auto ib(tmp_cont.begin()), ie(tmp_cont.end()); ib not_eq ie; ib++ )
	{
		pch = *ib;
		delete pch;
	}

	if ( pool ) delete pool;

	return nullptr;
}

MultiChannelPool::MultiChannelPool(const pw::MultiChannelPool::create_param_type& param) : m_poller(param.param.poller), m_factory(param.factory), m_tag(param.tag), m_reconnect_time(1*1000LL), m_pool_next(m_pool.end())
{
}

MultiChannelPool::~MultiChannelPool()
{
	ch_type* pch(nullptr);
	for (chpool_itr ib_pool(m_pool.begin()), ie_pool(m_pool.end()); ib_pool not_eq ie_pool; ib_pool++)
	{
		chgroup_type& grp(ib_pool->second);
		for ( chgroup_type::grp_itr ib_grp(grp.cont.begin()), ie_grp(grp.cont.end()); ib_grp not_eq ie_grp; ib_grp++ )
		{
			chhost_type& chhost(ib_grp->second);
			for ( chhost_type::ch_itr ib_ch(chhost.cont.begin()), ie_ch(chhost.cont.end()); ib_ch not_eq ie_ch; ib_ch++ )
			{
				pch = *ib_ch;
				pch->setExpired();
			}
		}
	}
}

size_t
MultiChannelPool::broadcastFull(const PacketInterface& pk, ch_list* pout)
{
	if ( pout ) pout->clear();
	size_t count(0);

	ch_type* pch(nullptr);
	for (chpool_itr ib_pool(m_pool.begin()), ie_pool(m_pool.end()); ib_pool not_eq ie_pool; ib_pool++)
	{
		chgroup_type& grp(ib_pool->second);
		for ( chgroup_type::grp_itr ib_grp(grp.cont.begin()), ie_grp(grp.cont.end()); ib_grp not_eq ie_grp; ib_grp++ )
		{
			chhost_type& chhost(ib_grp->second);
			for ( chhost_type::ch_itr ib_ch(chhost.cont.begin()), ie_ch(chhost.cont.end()); ib_ch not_eq ie_ch; ib_ch++ )
			{
				pch = *ib_ch;
				pch->write(pk);
				++count;
				if ( pout ) pout->push_back(pch);
			}
		}
	}

	return count;
}

size_t
MultiChannelPool::broadcastPerHost(const PacketInterface& pk, ch_list* pout)
{
	if ( pout ) pout->clear();
	size_t count(0);

	ch_type* pch(nullptr);
	for (chpool_itr ib_pool(m_pool.begin()), ie_pool(m_pool.end()); ib_pool not_eq ie_pool; ib_pool++)
	{
		chgroup_type& grp(ib_pool->second);
		for ( chgroup_type::grp_itr ib_grp(grp.cont.begin()), ie_grp(grp.cont.end()); ib_grp not_eq ie_grp; ib_grp++ )
		{
			chhost_type& chhost(ib_grp->second);
			if ( nullptr not_eq (pch = chhost.getNext()) )
			{
				pch->write(pk);
				++count;
				if ( pout ) pout->push_back(pch);
			}
		}
	}

	return count;
}

size_t
MultiChannelPool::broadcastPerGroup(const PacketInterface& pk, ch_list* pout)
{
	if ( pout ) pout->clear();
	size_t count(0);

	ch_type* pch(nullptr);
	for (chpool_itr ib_pool(m_pool.begin()), ie_pool(m_pool.end()); ib_pool not_eq ie_pool; ib_pool++)
	{
		chgroup_type& grp(ib_pool->second);
		if ( nullptr not_eq (pch = grp.getNext()) )
		{
			pch->write(pk);
			++count;
			if ( pout ) pout->push_back(pch);
		}
	}

	return count;
}

std::ostream&
MultiChannelPool::dump(std::ostream& os) const
{
	PWSHOWMETHOD();
	os << "MultiChannelPool(this: " << (void*)this << "): " << m_tag << std::endl;
	os << "Poller: " << (void*)m_poller << ' ' << (m_poller?typeid(*m_poller).name():"unknown") << std::endl;
	os << "Factory: " << (void*)m_factory << ' ' << (m_factory?typeid(*m_factory).name():"unknown") << std::endl;
	os << "Reconnect Time: " << m_reconnect_time << std::endl;
	os << "Pool(" << m_pool.size() << ')' << std::endl;

	ch_type* pch(nullptr);
	for (chpool_citr ib_pool(m_pool.begin()), ie_pool(m_pool.end()); ib_pool not_eq ie_pool; ib_pool ++ )
	{
		os << "\tGroup Name: " << ib_pool->first << std::endl;
		for (chgroup_type::grp_citr ib_grp(ib_pool->second.cont.begin()), ie_grp(ib_pool->second.cont.end()); ib_grp not_eq ie_grp; ib_grp++ )
		{
			const chhost_type& chhost(ib_grp->second);
			os << "\t\tHost: " << chhost.host.host.c_str() << ':' << chhost.host.service.c_str() << std::endl;
			for (chhost_type::ch_citr ib_ch(ib_grp->second.cont.begin()), ie_ch(ib_grp->second.cont.end()); ib_ch not_eq ie_ch; ib_ch++ )
			{
				pch = *ib_ch;
				//os << "\t\t\tChannel: " << (void*)pch << ' ' << (pch?typeid(*pch).name():"unknown") << std::endl;
				os << "\t\t\tChannel: ";
				pch->dump(os) << std::endl;
			}
		}
	}

	return os;
}

void
MultiChannelPool::add(MultiChannelInterface* pch)
{
	const size_t gname(pch->getGroupName());
	chpool_itr ib(m_pool.find(gname));
	if ( m_pool.end() == ib )
	{
		ib = m_pool.insert(chpool_cont::value_type(gname, chgroup_type(gname))).first;
	}

	ib->second.add(pch);

	PWTRACE("MultiChannel is added: tag:%s pch:%p pch-type:%s", m_tag.c_str(), pch, typeid(*pch).name());
}

void
MultiChannelPool::remove(MultiChannelInterface* pch)
{
	for ( auto& grp : m_pool )
	{
		for ( auto& host : grp.second.cont )
		{
			for ( auto& ch : host.second.cont )
			{
				if ( ch == pch )
				{
					PWTRACE("MultiChannel is removed: tag:%s pch:%p pch-type:%s", m_tag.c_str(), pch, typeid(*pch).name());

					host.second.getNext();
					host.second.cont.erase(ch);
					return;
				}
			}
		}
	}
}

MultiChannelPool::ch_type*
MultiChannelPool::getChannel(void)
{
	if ( m_pool.empty() ) return nullptr;

	ch_type* ret(nullptr);
	size_t count(0), total(m_pool.size());

	while ( (nullptr == (ret = m_pool_next->second.getNext())) and (count < total) )
	{
		++count;
		if ( m_pool.end() == ++m_pool_next ) m_pool_next = m_pool.begin();
	}

	if ( nullptr not_eq ret )
	{
		if ( m_pool.end() == ++m_pool_next ) m_pool_next = m_pool.begin();
	}

	return ret;
}

MultiChannelPool::ch_type*
MultiChannelPool::getChannel(size_t gname)
{
	chpool_itr ib(m_pool.find(gname));
	if ( m_pool.end() == ib ) return nullptr;
	return ib->second.getNext();
}

//------------------------------------------------------------------------------
// Multi Channel

MultiChannelInterface::MultiChannelInterface(const MultiChannelPool::create_param_type& param)
	: MsgChannel(param.param), m_ch_pool(param.pool), m_gname(param.gname), m_index(param.index), m_host(param.host), m_connected(false)
{
	PWSHOWMETHOD();
}

MultiChannelInterface::~MultiChannelInterface()
{
	PWSHOWMETHOD();
}

void
MultiChannelInterface::eventTimer(int id, void* param)
{
	switch(id)
	{
	case TIMER_RECONNECT_INIT:
	{
		PWTRACE("RECONNECT_INIT");
		TimerRemove(this, TIMER_RECONNECT_INIT);
		reconnect();
		return;
	}// TIMER_RECONNECT_INIT
	case TIMER_RECONNECT_RESPONSE:
	{
		PWTRACE("RECONNECT_RESPONSE_TIMEOUT");
		TimerRemove(this, TIMER_RECONNECT_RESPONSE);
		clearInstance();
		TimerAdd(this, TIMER_RECONNECT_INIT, m_ch_pool->getReconnectTime());
		return;
	}
	}; //switch(id)

	MsgChannel::eventTimer(id, param);
}

void
MultiChannelInterface::reconnect(void)
{
	PWSHOWMETHOD();
	if ( not connect(m_host) )
	{
		PWLOGLIB("failed to connect: host:%s:%s", m_host.host.c_str(), m_host.service.c_str());
		clearInstance();

		// 처음부터 다시 시작
		TimerAdd(this, TIMER_RECONNECT_INIT, m_ch_pool->getReconnectTime());
		return;
	}
}

void
MultiChannelInterface::eventError(Error type, int err)
{
	MsgChannel::eventError(type, err);

	setDisconnected();
}

void
MultiChannelInterface::eventConnect(void)
{
	MsgPacket pk;
	bool flag_send(true), flag_wait(true);
	if ( getHelloPacket(pk, flag_send, flag_wait) )
	{
		PWTRACE("getHelloPacket: flag_send:%d flag_wait:%d", int(flag_send), int(flag_wait));
		if ( flag_send )
		{
			write(pk);
		}

		if ( flag_wait )
		{
			TimerAdd(this, TIMER_RECONNECT_RESPONSE, m_ch_pool->getReconnectTime()*2);
		}
		else
		{
			PWTRACE("setConnected by pass");
			setConnected();
		}
	}
	else
	{
		setDisconnected();
	}
}

void
MultiChannelInterface::hookReadPacket(const MsgPacket& pk, const char* body, size_t bodylen)
{
	//PWSHOWMETHOD();
	if ( isConnected() )
	{
		//PWTRACE("isConnected!!!!!");
		eventReadPacket(pk, body, bodylen);
	}
	else
	{
		std::string sname;
		if ( checkHelloPacket(sname, pk) )
		{
			m_peer_name = sname;
			setConnected();
		}
		else
		{
			PWLOGLIB("failed to check hello packet with sync: host: %s:%s", m_host.host.c_str(), m_host.service.c_str());
			eventError(Error::INVALID_PACKET, 0);
		}
	}
}

bool
MultiChannelInterface::connectSync(void)
{
	PWSHOWMETHOD();
	typedef void (func_type) (int);
	func_type* oldALRM(::signal(SIGALRM, sigALRM));
	unsigned int left_time(::alarm(5));

	do {
		if ( not connect(m_host, PF_UNSPEC, false) )
		{
			PWLOGLIB("failed to connect with sync: host: %s:%s", m_host.host.c_str(), m_host.service.c_str());
			break;
		}

		MsgPacket pk;
		bool flag_send(true), flag_wait(true);
		if ( not getHelloPacket( pk, flag_send, flag_wait ) )
		{
			PWLOGLIB("failed to get hello packet: host: %s:%s", m_host.host.c_str(), m_host.service.c_str());
			break;
		}

		if ( flag_send )
		{
			std::string dump;
			pk.write(dump);

			if ( ssize_t(dump.size()) != ::send(m_fd, dump.c_str(), dump.size(), 0) )
			{
				PWLOGLIB("failed to connect with sync: host: %s:%s", m_host.host.c_str(), m_host.service.c_str());
				break;
			}
		}

		if ( flag_wait )
		{
			if ( not getPacketSync(pk) )
			{
				PWLOGLIB("failed to get packet with sync: host: %s:%s", m_host.host.c_str(), m_host.service.c_str());
				break;
			}

			std::string sname;
			if ( checkHelloPacket(sname, pk) )
			{
				m_peer_name = sname;
				setConnected();
			}
			else
			{
				PWLOGLIB("failed to check hello packet with sync: host: %s:%s", m_host.host.c_str(), m_host.service.c_str());
				break;
			}
		}
		else
		{
			setConnected();
		}

		::signal(SIGALRM, oldALRM);
		return true;
	} while (false);

	::signal(SIGALRM, oldALRM);
	if ( left_time ) ::alarm(left_time);

	clearInstance();

	return false;
}

void
MultiChannelInterface::disconnect(void)
{
	PWTRACE("disconnected!");
	if ( not m_connected ) return;

	//m_ch_pool->remove(this);
	clearInstance();

	TimerRemove(this, TIMER_RECONNECT_INIT);
	TimerRemove(this, TIMER_RECONNECT_RESPONSE);
}

void
MultiChannelInterface::setConnected(void)
{
	PWTRACE("%s %p", __func__, this);

	if ( m_connected ) return;

	m_connected = true;
	//m_ch_pool->add(this);
	TimerRemove(this, TIMER_RECONNECT_INIT);
	TimerRemove(this, TIMER_RECONNECT_RESPONSE);

	eventConnected();
}

void
MultiChannelInterface::setDisconnected(void)
{
	PWTRACE("%s %p", __func__, this);

	const bool bc(m_connected);

	m_connected = false;

	if ( bc ) eventDisconnected();

	//m_ch_pool->remove(this);

	clearInstance();

	TimerAdd(this, TIMER_RECONNECT_INIT, m_ch_pool->getReconnectTime());
}

void
MultiChannelInterface::eventPingTimeout(void)
{
	auto diff(this->getDiffFromLastRead());
	PWLOGLIB("eventPingTimeout: diff:%jd this:%p type:%s", diff, this, typeid(*this).name());
	setDisconnected();
}

std::ostream&
MultiChannelInterface::dump(std::ostream& os) const
{
	os << "this: " << (void*)this
		<< " chpool: " << (void*)m_ch_pool
		<< " index: " << m_index
		<< " gname: " << m_gname
		<< " host: " << m_host.host.c_str() << ':' << m_host.service.c_str()
		<< " connected: " << m_connected << " peername: " << m_peer_name;
	return os;
}

//------------------------------------------------------------------------------
// Multi Channel Factory
MultiChannelFactory::~MultiChannelFactory()
{
}

void
MultiChannelFactory::release(MultiChannelInterface* pch)
{
	if ( pch ) delete pch;
}

};//namespace pw;
