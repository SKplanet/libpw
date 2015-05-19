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

#include "./pw_channel_if.h"
#include "./pw_ssl.h"
#include "./pw_timer.h"
#include "./pw_log.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace pw
{

#define CASE_CSTR_RETURN(x)	case x: return #x

static ChannelMapTemplate<ChannelInterface> s_channels;

chif_create_type::chif_create_type (int _fd, IoPoller* _poller, const SslContext* _ctx, size_t _bufsize, void* _append) : fd(_fd), poller(_poller), ssl(_ctx ? Ssl::s_create(_ctx):nullptr), bufsize(_bufsize), append(_append)
{
}

std::ostream&
chif_create_type::dump ( std::ostream& os ) const
{
	os << "chif_create_type(" << this <<
		") fd: " << fd <<
		" poller: " << poller <<
		" ssl: " << ssl <<
		" bufsize: " << bufsize <<
		" append: " << append << endl;

	return os;
}

ChannelInterface*
ChannelInterface::s_getChannel(ch_name_type unique_name)
{
	return s_channels.get(unique_name);
}

const char*
ChannelInterface::s_toString(Error e)
{
	switch(e)
	{
	CASE_CSTR_RETURN(Error::CONNECT);
	CASE_CSTR_RETURN(Error::EX_HANDSHAKING);
	CASE_CSTR_RETURN(Error::INVALID_PACKET);
	CASE_CSTR_RETURN(Error::NORMAL);
	CASE_CSTR_RETURN(Error::READ);
	CASE_CSTR_RETURN(Error::READ_CLOSE);
	CASE_CSTR_RETURN(Error::SSL_HANDSHAKING);
	CASE_CSTR_RETURN(Error::WRITE);
	}

	return "Error::UNKNOWN";
}

const char*
ChannelInterface::s_toString(InstanceState i)
{
	switch(i)
	{
	CASE_CSTR_RETURN(InstanceState::DELETE);
	CASE_CSTR_RETURN(InstanceState::EXPIRED);
	CASE_CSTR_RETURN(InstanceState::NORMAL);
	}

	return "InstanceState::UNKNOWN";
}

const char*
ChannelInterface::s_toString(ConnectState c)
{
	switch(c)
	{
	CASE_CSTR_RETURN(ConnectState::EX_HANDSHAKING);
	CASE_CSTR_RETURN(ConnectState::FAIL);
	CASE_CSTR_RETURN(ConnectState::NONE);
	CASE_CSTR_RETURN(ConnectState::SEND);
	CASE_CSTR_RETURN(ConnectState::SSL_HANDSHAKING);
	CASE_CSTR_RETURN(ConnectState::SUCC);
	}

	return "ConnectState::UNKNOWN";
}

const char*
ChannelInterface::s_toString(Check c)
{
	switch(c)
	{
	CASE_CSTR_RETURN(Check::NONE);
	CASE_CSTR_RETURN(Check::READ);
	CASE_CSTR_RETURN(Check::WRITE);
	CASE_CSTR_RETURN(Check::BOTH);
	}

	return "Check::UNKNOWN";
}

const char*
ChannelInterface::s_toString(RecvState r)
{
	switch(r)
	{
	CASE_CSTR_RETURN(RecvState::START);
	CASE_CSTR_RETURN(RecvState::FIRST_LINE);
	CASE_CSTR_RETURN(RecvState::HEADER);
	CASE_CSTR_RETURN(RecvState::BODY);
	CASE_CSTR_RETURN(RecvState::DONE);
	CASE_CSTR_RETURN(RecvState::ERROR);
	}

	return "RecvState::UNKNOWN";
}

ChannelInterface::ChannelInterface() :
	Socket(-1, nullptr),
	m_ssl(nullptr),
	m_rbuf(nullptr),
	m_wbuf(nullptr),
//	m_inst_state(InstanceState::NORMAL),
//	m_conn_state(ConnectState::NONE),
//	m_check_type(Check::NONE),
	m_unique_name(s_channels.insert(this))
{
	do
	{
		m_rbuf = new IoBuffer(IoBuffer::DEFAULT_SIZE, IoBuffer::DEFAULT_DELTA);
		m_wbuf = new IoBuffer(IoBuffer::DEFAULT_SIZE, IoBuffer::DEFAULT_DELTA);

		if ( (not m_rbuf) or (not m_wbuf) )
		{
			PWLOGLIB("failed to create buffer: channel:%p", this);
			break;
		}
	} while (false);
}

ChannelInterface::ChannelInterface(const chif_create_type& param) :
	Socket(param.fd, param.poller),
	m_ssl(param.ssl),
	m_rbuf(nullptr),
	m_wbuf(nullptr),
//	m_inst_state(InstanceState::NORMAL),
//	m_conn_state(ConnectState::NONE),
//	m_check_type(Check::NONE),
	m_unique_name(s_channels.insert(this))
{
	do
	{
		if ( m_ssl )
		{
			PWTRACE("SSL type");
			m_rbuf = new IoBufferSsl(param.bufsize, IoBuffer::DEFAULT_DELTA, m_ssl);
			m_wbuf = new IoBufferSsl(IoBuffer::DEFAULT_SIZE, IoBuffer::DEFAULT_DELTA, m_ssl);
		}
		else
		{
			PWTRACE("PLAIN type");
			m_rbuf = new IoBuffer(param.bufsize, IoBuffer::DEFAULT_DELTA);
			m_wbuf = new IoBuffer(IoBuffer::DEFAULT_SIZE, IoBuffer::DEFAULT_DELTA);
		}

		if ( (not m_rbuf) or (not m_wbuf) )
		{
			PWLOGLIB("failed to create buffer: channel:%p", this);
			break;
		}
	} while (false);

	if ( m_fd >= 0 )
	{
		setConnSuccess();
	}
}

ChannelInterface::~ChannelInterface()
{
	if ( m_rbuf ) { delete m_rbuf; m_rbuf = nullptr; }
	if ( m_wbuf ) { delete m_wbuf; m_wbuf = nullptr; }
	if ( m_ssl ) { Ssl::s_release(m_ssl); m_ssl = nullptr; }

	s_channels.erase(m_unique_name);
}

std::ostream&
ChannelInterface::dump(std::ostream& os) const
{
	os << "addr: " << this << " type/name: " << typeid(*this).name() << '/'
		<< m_unique_name << " m_ssl: " << m_ssl << " m_rbuf: "
		<< (m_rbuf ? typeid(*m_rbuf).name() : "nullptr") << ' ' << m_rbuf
		<< " m_wbuf: " << (m_wbuf ? typeid(*m_wbuf).name() : "nullptr") << ' '
		<< m_wbuf << " m_inst_state: " << s_toString(m_inst_state)
		<< " m_conn_state: " << s_toString(m_conn_state) << " m_recv_state: "
		<< s_toString(m_recv_state) << " m_check_type: "
		<< s_toString(m_check_type);

	return os;
}

bool
ChannelInterface::connectEx(void)
{
	if ( m_fd < 0 ) return false;

	int revent(0);
	if ( this->procConnectEx(revent) )
	{
		setConnSuccess();
		if ( m_poller ) m_poller->setMask(m_fd, POLLIN);
		return true;
	}

	if ( (not revent) or (not m_poller) )
	{
		setConnFail();
		errno = ECANCELED;
		return false;
	}

	setConnExHandShaking();
	m_poller->setMask(m_fd, revent);
	errno = EINPROGRESS;

	return false;
}

bool
ChannelInterface::acceptEx(void)
{
	if ( m_fd < 0 ) return false;

	int revent(0);
	if ( this->procAcceptEx(revent) )
	{
		setConnSuccess();
		if ( m_poller ) m_poller->setMask(m_fd, POLLIN);
		return true;
	}

	if ( (not revent) or (not m_poller) )
	{
		setConnFail();
		errno = ECANCELED;
		return false;
	}

	setConnExHandShaking();
	m_poller->setMask(m_fd, revent);
	errno = EINPROGRESS;

	return false;
}

bool
ChannelInterface::handshakeEx(void)
{
	if ( not isConnSslHandShaking() )
	{
		PWLOGLIB("connection status is not ex handshaking: this:%p fd:%d conn_state:%d", this, m_fd, m_conn_state);
		return false;
	}

	if ( m_fd < 0 ) return false;

	int revent(0);
	if ( this->procHandshakeEx(revent) )
	{
		setConnSuccess();
		if ( m_poller ) m_poller->setMask(m_fd, POLLIN);
		return true;
	}

	if ( (not revent) or (not m_poller) )
	{
		setConnFail();
		errno = ECANCELED;
		return false;
	}

	setConnExHandShaking();
	m_poller->setMask(m_fd, revent);
	errno = EINPROGRESS;

	return false;
}

bool
ChannelInterface::connectSsl(Error* errpos)
{
	if ( (not m_ssl) or (m_fd < 0) )
	{
		PWTRACE("m_ssl:%p m_fd:%d is not valid", m_ssl, m_fd);
		return false;
	}

	m_ssl->reset();
	m_ssl->setFD(m_fd);

	int revent(0);
	if ( m_ssl->connect(&revent) )
	{
		if ( this->isExHandShakingChannel() )
		{
			if ( not connectEx() )
			{
				if ( errpos )
				{
					PWTRACE("connectEx is failed");
					*errpos = Error::EX_HANDSHAKING;
				}

				return false;
			}

			return true;
		}

		setConnSuccess();
		if ( m_poller ) m_poller->setMask(m_fd, POLLIN);
		return true;
	}

	if ( (not revent) or (not m_poller) )
	{
		PWTRACE("revent:%d m_poller:%p", revent, m_poller);
		setConnFail();
		errno = ECANCELED;
		if ( errpos ) *errpos = Error::SSL_HANDSHAKING;
		return false;
	}

	setConnSslHandShaking();
	m_poller->setMask(m_fd, revent);
	errno = EINPROGRESS;
	if ( errpos )
	{
		*errpos = Error::SSL_HANDSHAKING;
	}

	return false;
}

bool
ChannelInterface::acceptSsl(Error* errpos)
{
	if ( (not m_ssl) or (m_fd < 0) ) return false;

	m_ssl->reset();
	m_ssl->setFD(m_fd);

	int revent(0);
	if ( m_ssl->accept(&revent) )
	{
		if ( this->isExHandShakingChannel() )
		{
			if ( not connectEx() )
			{
				if ( errpos ) *errpos = Error::EX_HANDSHAKING;
				return false;
			}

			return true;
		}

		setConnSuccess();
		if ( m_poller ) m_poller->setMask(m_fd, POLLIN);
		return true;
	}

	if ( (not revent) or (not m_poller) )
	{
		PWTRACE("revent:%d m_poller:%p", revent, m_poller);
		setConnFail();
		errno = ECANCELED;
		if ( errpos ) *errpos = Error::SSL_HANDSHAKING;
		return false;
	}

	PWTRACE("in progress...: %d", revent);
	setConnSslHandShaking();
	m_poller->setMask(m_fd, revent);
	errno = EINPROGRESS;
	if ( errpos ) *errpos = Error::SSL_HANDSHAKING;

	return false;
}

bool
ChannelInterface::handshakeSsl(Error* errpos)
{
	if ( not isConnSslHandShaking() )
	{
		PWLOGLIB("connection status is not ssl handshaking: this:%p fd:%d conn_state:%d", this, m_fd, m_conn_state);
		return false;
	}

	if ( (not m_ssl) or (m_fd < 0) )
	{
		PWTRACE("m_ssl:%p m_fd:%d is not valid", m_ssl, m_fd);
		return false;
	}

	int revent(0);
	if ( m_ssl->handshake(&revent) )
	{
		if ( this->isExHandShakingChannel() )
		{
			if ( not connectEx() )
			{
				if ( errpos ) *errpos = Error::EX_HANDSHAKING;
				return false;
			}

			return true;
		}

		setConnSuccess();
		if ( m_poller ) m_poller->setMask(m_fd, POLLIN);
		return true;
	}

	if ( (not revent) or (not m_poller) )
	{
		PWTRACE("revent:%d m_poller:%p", revent, m_poller);
		//PWTRACE("ssl error: %s", ssl::getLastErrorString());
		setConnFail();
		errno = ECANCELED;
		if ( errpos ) *errpos = Error::SSL_HANDSHAKING;
		return false;
	}

	PWTRACE("in progress...: %d", revent);
	m_poller->setMask(m_fd, revent);
	errno = EINPROGRESS;
	if ( errpos ) *errpos = Error::SSL_HANDSHAKING;

	return false;
}

bool
ChannelInterface::procConnect(const char* host, const char* service, int family, bool async)
{
	if ( isConnSuccess() or isConnSend() or (m_fd >= 0) )
	{
		PWLOGLIB("confused connection state: this:%p conn_state:%d fd:%d", this, m_conn_state, m_fd);
		return false;
	}

	setConnNone();

	errno = 0;
	do {
		if ( Socket::s_connect(m_fd, host, service, family, async) )
		{
			PWTRACE("connected!: m_ssl:%p", m_ssl);
			// SSL이 세팅되어 있으면, 핸드쉐이킹으로 넘어간다.
			if ( m_ssl )
			{
				if ( this->connectSsl() ) return true;
				else if ( async and (errno == EINPROGRESS) )
				{
					PWTRACE("Ssl in progress... %s:%s", host, service);
					return true;
				}

				/* 이도저도 아니면 자원 정리 */
				break;
			}
			else if ( this->isExHandShakingChannel() )
			{
				if ( this->connectEx() ) return true;
				else if ( async and (errno == EINPROGRESS) )
				{
					PWTRACE("Ex in progress... %s:%s", host, service);
					return true;
				}

				/* 이도저도 아니면 자원 정리 */
				break;
			}

			this->setConnSuccess();
			m_poller->setMask(m_fd, POLLIN);

			if ( async ) hookConnect();

			return true;
		}
		else if ( async and (errno == EINPROGRESS) and (m_fd >= 0) )
		{
			PWTRACE("in progress... %s:%s", host, service);
			setConnSend();
			if ( m_poller ) m_poller->add(m_fd, this, POLLOUT);

			return true;
		}
	} while (false);

	return false;
}

void
ChannelInterface::clearInstance(void)
{
	if ( m_fd > 0 )
	{
		if ( m_poller ) m_poller->remove(m_fd);
		::close(m_fd);
		m_fd = -1;
	}

	m_inst_state = InstanceState::NORMAL;
	setConnNone();

	m_rbuf->clear();
	m_wbuf->clear();
}

void
ChannelInterface::setExpired(void)
{
	PWTRACE("setExpired: pid:%d this:%p", getpid(), this);
	if ( isInstDeleteOrExpired() )
	{
		PWTRACE("setExpired: already");
		return;
	}

	m_inst_state = InstanceState::EXPIRED;

	if ( m_fd != -1 )
	{
		if ( m_poller )
		{
			m_poller->setMask(m_fd, POLLOUT);
			return;
		}
	}

	PWLOGLIB("setExpired, but no poller or no fd: this:%p poller:%p fd:%d", this, m_poller, m_fd);
}

void
ChannelInterface::setRelease(void)
{
	if ( isInstDelete() )
	{
		//PWTRACE("Already release state");
		return;
	}

	m_inst_state = InstanceState::DELETE;

	if ( m_fd != -1 )
	{
		if ( m_poller )
		{
			//PWTRACE("set pollout");
			m_poller->setMask(m_fd, POLLOUT);
			return;
		}
	}

	PWLOGLIB("setRelease, but no poller or no fd: this:%p poller:%p fd:%d", this, m_poller, m_fd);
}

void
ChannelInterface::close(void)
{
	if ( m_ssl ) m_ssl->reset();
	Socket::close();
	setConnNone();
}

void
ChannelInterface::destroy(void)
{
	delete this;
}

void
ChannelInterface::releaseInstance(void)
{
	if ( m_fd >= 0 ) close();

	destroy();
}

void
ChannelInterface::eventIo(int fd, int event, bool&/* del_event */)
{
	if ( isInstDelete() )
	{
		releaseInstance();
		return;
	}

	do {
		if ( not isConnSuccess() )
		{
			eventConnecting();
			break;
		}

		if( event bitand POLLIN )
		{
			if ( isInstExpired() )
			{
				m_poller->setMask(m_fd, POLLOUT);
			}
			else
			{
				eventRead(event);
			}
			break;
		}

		if ( isInstDelete() ) break;

		if ( event bitand POLLOUT )
		{
			eventWrite(event);
			break;
		}

		//if ( isInstDelete() ) break;
		if ( event bitand (POLLERR bitor POLLHUP bitor POLLNVAL) )
		{
			eventError(Error::NORMAL, errno);
			break;
		}
	} while(false);

	if ( isInstDelete() ) releaseInstance();
}

void
ChannelInterface::eventConnecting(void)
{
	//PWSHOWMETHOD();
	if ( isConnSend() )
	{
		int err(0);
		if ( isConnected(&err) )
		{
			if ( m_ssl )
			{
				//PWTRACE("ssl handshaking");
				Error errpos;
				if ( this->connectSsl(&errpos) )
				{
					hookConnect();
				}
				else if ( errno not_eq EINPROGRESS )
				{
					PWTRACE("errno is not EINPROGRESS");
					eventError(errpos, errno);
				}
			}//if (m_ssl)
			else if ( isExHandShakingChannel() )
			{
				//PWTRACE("ex handshaking");
				if ( this->connectEx() )
				{
					hookConnect();
				}
				else if ( errno not_eq EINPROGRESS )
				{
					eventError(Error::EX_HANDSHAKING, errno);
				}
			}// else if ( isExHandshakingChannel() )
			else
			{
				//PWTRACE("or...?");
				int err(0);
				if ( not Socket::isConnected(&err) )
				{
					PWLOGLIB("Invalid process: conn_state: %s inst_state: %s err:%d errno:%d", s_toString(m_conn_state), s_toString(m_inst_state), err, errno);
					eventError(Error::CONNECT, errno);
					return;
				}

				this->setConnSuccess();
				m_poller->setMask(m_fd, POLLIN);
				hookConnect();
			}

			return;
		}// if ( isConnected(..) )
		else
		{
			//PWTRACE("what?");
			eventError(Error::CONNECT, err?err:errno);
		}
	}//isConnSend()
	else if ( isConnSslHandShaking() )
	{
		PWTRACE("continued ssl handshaking");
		Error errpos;
		if ( this->handshakeSsl(&errpos) )
		{
			hookConnect();
		}
		else if ( errno not_eq EINPROGRESS )
		{
			eventError(errpos, errno);
		}

		return;
	}
	else if ( isConnExHandShaking() )
	{
		PWTRACE("continued ex handshaking");
		if ( this->handshakeEx() )
		{
			hookConnect();
		}
		else if ( errno not_eq EINPROGRESS )
		{
			eventError(Error::EX_HANDSHAKING, errno);
		}

		return;
	}
	else
	{
		PWLOGLIB("Invalid process: conn_state: %s inst_state: %s", s_toString(m_conn_state), s_toString(m_inst_state));
		eventError(Error::CONNECT, errno);
	}
}

void
ChannelInterface::eventRead(int event)
{
	//PWSHOWMETHOD();
	ssize_t len;
	if ( (len = m_rbuf->readFromFile(m_fd)) > 0 )
	{
		eventReadData(size_t(len));
	}
	else if ( 0 == len )
	{
		//PWTRACE("not yet?");
		eventError(Error::READ_CLOSE, errno);
	}
	else if ( s_isAgain(errno) )
	{
		return;
	}
	else
	{
		eventError(Error::READ, errno);
	}
}

void
ChannelInterface::eventWrite(int event)
{
	if ( not isConnSuccess() )
	{
		PWTRACE("lost connection");
		return;
	}

	if ( m_wbuf->isEmpty() )
	{
		if ( isInstExpired() )
		{
			//PWTRACE("expired! setRelease!");
			setRelease();
		}
		else
		{
			m_poller->setMask(m_fd, POLLIN);
			// 아래 구문 대신 setMask로 대체
			//m_poller->andMask(m_fd, ~POLLOUT);
		}

		return;
	}

	const size_t blen(m_wbuf->getReadableSize());
	if ( isCheckWrite() )
	{
		if ( blen > SOCKBUF_SIZE_CHECK )
		{
			eventOverflow(POLLOUT, blen, SOCKBUF_SIZE_CHECK);
		}
	}

	const size_t count(getEventDispatchCount());
	size_t i(0);
	ssize_t len(0);
	while ( i < count )
	{
		++i;

		if ( (len = m_wbuf->writeToFile(m_fd)) > 0 )
		{
			//PWTRACE("writeToFile");
			eventWriteData(size_t(len));
			m_wbuf->flush();
			if ( m_wbuf->isEmpty() )
			{
				if ( isInstExpired() ) setRelease();
				else
				{
					m_poller->setMask(m_fd, POLLIN);
					// 아래 구문 대신 setMask로 대체
					//m_poller->andMask(m_fd, ~POLLOUT);
				}

				break;
			}// if ( m_wbuf->empty() )
		}
		else
		{
			if ( s_isAgain(errno) ) break;
			m_wbuf->clear();
			eventError(Error::WRITE, errno);
			break;
		}
	}// while ( i < count )
}

void
ChannelInterface::eventOverflow(int event, size_t nowlen, size_t maxlen)
{
	PWLOGLIB("Socket overflow: fd:%d event:%d nowlen:%zu maxlen:%zu", m_fd, event, nowlen, maxlen);
}

void
ChannelInterface::eventError(Error type, int err)
{
	if ( type not_eq Error::SSL_HANDSHAKING )
	{
		PWTRACE("eventError: this:%p this.type:%s type:%s err:%d %s", this, typeid(*this).name(), s_toString(type), err, strerror(err));
	}
	else
	{
		PWTRACE("eventError: this:%p this.type:%s type:%s err:%d %s %s", this, typeid(*this).name(), s_toString(type), err, strerror(err), ssl::getLastErrorString());
	}

	setRelease();
}

void
ChannelInterface::eventConnect(void)
{
	PWTRACE("eventConnect: this:%p type:%s", this, typeid(*this).name());
}

bool
ChannelInterface::write(const PacketInterface& pk)
{
	if ( isInstDeleteOrExpired() ) return false;
	if ( (m_fd == -1) or (m_poller == nullptr) or (m_wbuf == nullptr) ) return false;
	if ( pk.write(*m_wbuf) <= 0 ) return false;
	m_poller->orMask(m_fd, POLLOUT);

	return true;
}

bool
ChannelInterface::write(const char* buf, size_t blen)
{
	if ( isInstDeleteOrExpired() ) return false;
	if ( (m_fd == -1) or (m_poller == nullptr) or (m_wbuf == nullptr) ) return false;
	if ( size_t(m_wbuf->writeToBuffer(buf, blen)) != blen ) return false;
	m_poller->orMask(m_fd, POLLOUT);

	return true;
}

bool
ChannelInterface::getLineSync(std::string& out, size_t limit/* = size_t(-1)*/)
{
	// Sync이며 1바이트씩 읽으면서 처리.
	// 이거 좀 위험한게 롤백을 할 수 없으니, 인생은 알아서 책임지세요.

	std::string outbuf;
	char c;
	int res(0);
	size_t retry(0);
	bool ret(false);

	while ( outbuf.size() < limit )
	{
		if ( ( res = ::read(m_fd, &c, 1) ) == 1 )
		{
			if ( c == char('\n') && not outbuf.empty() )
			{
				if ( outbuf[outbuf.size()-1] == '\r' )
				{
					outbuf.erase(outbuf.size()-1);
					ret = true;
					break;
				}
			}

			outbuf.push_back(c);
		}
		else if ( res == 0 )
		{
			PWTRACE("socket is closed");
			break;
		}
		else if ( s_isAgain(errno) )
		{
			++retry;
			if ( retry > 0 && (retry % 10) == 0 )
			{
				PWTRACE("too many retry for getLineSync");
			}

			::usleep(1000);
		}
		else
		{
			PWTRACE("socket error");
			break;
		}
	}

	outbuf.swap(out);
	return ret;
}

bool
ChannelInterface::getDataSync(std::string& out, size_t limit)
{
	// Sync이며 1바이트씩 읽으면서 처리.
	// 이거 좀 위험한게 롤백을 할 수 없으니, 인생은 알아서 책임지세요.

	std::string outbuf;
	char c;
	int res(0);
	size_t retry(0);

	while ( outbuf.size() < limit )
	{
		if ( ( res = ::read(m_fd, &c, 1) ) == 1 )
		{
			outbuf.push_back(c);
		}
		else if ( res == 0 )
		{
			PWTRACE("socket is closed");
			break;
		}
		else if ( s_isAgain(errno) )
		{
			++retry;
			if ( retry > 0 && (retry % 10) == 0 )
			{
				PWTRACE("too many retry for getDataSync");
			}

			::usleep(1000);
		}
		else
		{
			PWTRACE("socket error");
			break;
		}
	}

	outbuf.swap(out);
	return out.size() == limit;
}

bool
ChannelInterface::getDataSync(void* out, size_t limit)
{
	// Sync이며 1바이트씩 읽으면서 처리.
	// 이거 좀 위험한게 롤백을 할 수 없으니, 인생은 알아서 책임지세요.

	char* ib((char*)out);
	char* ie(ib+limit);
	char c;
	int res(0);
	size_t retry(0);

	while ( ib not_eq ie )
	{
		if ( ( res = ::read(m_fd, &c, 1) ) == 1 )
		{
			*ib = c;
		}
		else if ( res == 0 )
		{
			PWTRACE("socket is closed");
			break;
		}
		else if ( s_isAgain(errno) )
		{
			++retry;
			if ( retry > 0 && (retry % 10) == 0 )
			{
				PWTRACE("too many retry for getDataSync");
			}

			::usleep(1000);
		}
		else
		{
			PWTRACE("socket error");
			break;
		}

		++ib;
	}

	return ib == ie;
}

//namespace pw
}

