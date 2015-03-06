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
 * \file pw_sysinfo.h
 * \brief System information.
 * \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
 * \license This project is released under the MIT License.
 */

#include "./pw_common.h"

#ifndef __PW_SYSINFO_H__
#define __PW_SYSINFO_H__

namespace pw {

class SystemInformation final
{
public:
	//! \brief OS 간략정보
	using os_type = struct os_type
	{
		std::string name;		//!< OS 이름: Linux, FreeBSD
		std::string node;		//!< 호스트 이름
		std::string release;	//!< 릴리즈 이름
		std::string version;	//!< 버전 정보
		std::string machine;	//!< 머신 정보: i386, x86_64

		inline void clear(void)
		{
			name.clear();
			node.clear();
			release.clear();
			version.clear();
			machine.clear();
		}
	};

	//! \brief CPU 간략정보
	using cpu_type = struct cpu_type
	{
		size_t		index;	//!< Index
		double		freq;	//!< Frequency

		inline cpu_type() : index(0), freq(0.0) {}

		inline void clear(void)
		{
			index = 0;
			freq = 0.0;
		}
	};

	//! \brief CPU 정보 컨테이너
	using cpu_cont = std::list<cpu_type>;

	//! \brief NIC 간략정보
	using nic_type = struct nic_type
	{
		size_t		index;
		std::string	name;
		std::string	hwaddr;
		std::string	addr;
		std::string	mask;
		std::string	baddr;
		int			metric;
		int			mtu;

		struct
		{
			bool up;
			bool broadcast;
			bool debug;
			bool loopback;
			bool p2p;
			bool no_trailers;
			bool running;
			bool no_arp;
			bool promisc;
		} flags;

		inline void clear(void)
		{
			index = 0;
			name.clear();
			hwaddr.clear();
			addr.clear();
			mask.clear();
			baddr.clear();
			metric = 0;
			mtu = 0;

			memset(&flags, 0x00, sizeof(flags));
		}
	};

	//! \brief NIC 정보 컨테이너
	using nic_cont = std::list<nic_type>;

	//! \brief 메모리 간략정보
	//	Memory와 Swap정보를 통합하여 표기
	using memory_type = struct memory_type
	{
		struct
		{
			size_t	total;	//!< Total
			size_t	free;	//!< Free
		} mem, swap;

		inline void clear(void)
		{
			memset(&mem, 0x00, sizeof(mem));
			memset(&swap, 0x00, sizeof(swap));
		}
	};

	//! \brief 파일시스템 간략정보
	using fs_type = struct fs_type
	{
		std::string	name;
		size_t		total;
		size_t		free;
	};

	//! \brief 파일시스템 정보 컨테이너
	using fs_cont = std::map<std::string, fs_type>;

public:
	//! \brief OS 정보 얻기
	static bool s_getOS(os_type& out);

	//! \brief CPU 정보 얻기
	static bool s_getCPU(cpu_cont& out);

	//! \brief 메모리 정보 얻기
	static bool s_getMemory(memory_type& out);

	//! \brief NIC 정보 얻기
	static bool s_getNIC(nic_cont& out);

	//! \brief 파일 시스템 정보 얻기
	static bool s_getFileSystem(fs_cont& out);

	//! \brief 파일 시스템 정보 얻기
	static bool s_getFileSystem(fs_type& out, const char* mp);

public:
	//! \brief 전체 시스템 정보 얻기
	bool getAll(void);

public:
	os_type		m_os;
	cpu_cont	m_cpu;
	memory_type	m_memory;
	nic_cont	m_nic;
	fs_cont		m_fs;
};

};

#endif//__PW_SYSINFO_H__
