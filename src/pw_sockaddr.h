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
 * \file pw_sockaddr.h
 * \brief Support socket address.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_SOCKADDR_H__
#define __PW_SOCKADDR_H__

namespace pw {

//! \brief 소켓 주소 관리 클래스
class SocketAddress final
{
public:
	enum
	{
		MAX_STORAGE_SIZE = 128,	//!< 소켓 주소 최대 크기
		MAX_HOST_SIZE = 64,		//!< 호스트 문자열 최대 크기
		MAX_SERVICE_SIZE = 16,	//!< 서비스(포트) 문자열 최대 크기
		MAX_PATH_SIZE = 108,	//!< UNIX 소켓 패스 최대 크기
	};

	//! \brief 소켓 리스트 타입.
	using list_type = std::list<SocketAddress>;

public:
	static bool s_parseName(list_type& out, const char* host, const char* service, int family, int socktype, int protocol);

public:
	size_t recalculateSize(void);
	inline size_t getSize(void) const { return m_slen; }
	bool getName(char* host, size_t hlen, char* service, size_t slen, int* err = nullptr, int flag = s_default_get_name_flag ) const;
	bool getName(std::string* host, std::string* port, int* err = nullptr, int flag = s_default_get_name_flag ) const;
	inline bool getName(host_type& out, int* err = nullptr, int flag = s_default_get_name_flag ) const
	{
		std::string host, port;
		if ( this->getName(&host, &port, err, flag) )
		{
			out.host = host;
			out.service = port;
			return true;
		}

		return false;
	}
	inline void* getData(void) { return (void*)m_sa; }
	inline const void* getData(void) const { return (void*)m_sa; }
	void clear(void);

	void setFamily(int family);
	int getFamily(void) const;

	bool setIP(int family, const char* host, const char* service);
	bool getIP(int* family, char* host, char* service) const;
	inline bool getIP(int* family, std::string* host, std::string* service) const
	{
		char _host[MAX_HOST_SIZE];
		char _service[MAX_SERVICE_SIZE];

		if ( !this->getIP(family, _host, _service) ) return false;

		if ( host ) host->assign(_host);
		if ( service ) service->assign(_service);

		return true;
	}

	inline bool setIP4(const char* host, const char* service)
	{
		return this->setIP(PF_INET, host, service);
	}

	inline bool getIP4(char* host, char* service) const
	{
		int family(PF_INET);
		if ( this->getIP(&family, host, service) ) return ( family == PF_INET );
		return false;
	}

	inline bool getIP4(std::string* host, std::string* service) const
	{
		int family(PF_INET);
		if ( this->getIP(&family, host, service) ) return ( family == PF_INET );
		return false;
	}

	inline bool setIP6(const char* host, const char* service)
	{
		return this->setIP(PF_INET6, host, service);
	}

	inline bool getIP6(char* host, char* service) const
	{
		int family(PF_INET6);
		if ( this->getIP(&family, host, service) ) return ( family == PF_INET6 );
		return false;
	}

	inline bool getIP6(std::string* host, std::string* service) const
	{
		int family(PF_INET6);
		if ( this->getIP(&family, host, service) ) return ( family == PF_INET6 );
		return false;
	}

	int getPort(void) const;

	bool setPath(const char* path);
	bool getPath(char* path) const;
	bool getPath(std::string& path) const;
	char* getPath(void);
	const char* getPath(void) const;

	bool assignByPeer(int fd);
	bool assignBySocket(int fd);

	bool assign(const void* sa);
	bool assign(const void* sa, size_t slen);
	bool assign(const SocketAddress& sa);

public:
	inline virtual ~SocketAddress() {}
	inline explicit SocketAddress() : m_slen(0) {}
	inline SocketAddress(const void* sa) : m_slen(0) { this->assign(sa); }
	inline SocketAddress(const void* sa, size_t slen) : m_slen(0) { this->assign(sa, slen); }
	inline SocketAddress(const SocketAddress& sa) : m_slen(0) { this->assign(sa); }

public:
	static const int s_default_get_name_flag;	//!< getnameinfo 전용 플래그

private:
	bool _setIP4(const char* host, const char* service);
	bool _setIP6(const char* host, const char* service);

private:
	char	m_sa[MAX_STORAGE_SIZE] = {0x00, };
	size_t	m_slen;
};

};//namespace pw

#endif//!__PW_SOCKADDR_H__

