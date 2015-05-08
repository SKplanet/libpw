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

#include "./pw_sysinfo.h"
#include "./pw_string.h"
#include "./pw_sockaddr.h"
#include "./pw_log.h"

#include <sys/utsname.h>
#include <sys/sysctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <sys/statvfs.h>

#if defined(OS_BSD)
#	include <vm/vm_param.h>
#	include <sys/param.h>
#	include <sys/ucred.h>
#	include <sys/mount.h>
#	include <fstab.h>
#endif//OS_BSD

#if defined(OS_APPLE)
#	include <fstab.h>
#endif//OS_APPLE

#if defined(OS_LINUX)
#	include <mntent.h>
#endif//OS_LINUX

#if defined(HAVE_NET_IF_H)
#	include <net/if.h>
#endif

#if defined(HAVE_IFADDRS_H)
#	include <ifaddrs.h>
#endif

namespace pw {

bool
SystemInformation::s_getOS(os_type& os)
{
	struct utsname uts;
	if ( -1 == ::uname(&uts) ) return false;

	os.name = uts.sysname;
	os.node = uts.nodename;
	os.release = uts.release;
	os.version = uts.version;
	os.machine = uts.machine;

	return true;
}

bool
SystemInformation::s_getCPU(cpu_cont& out)
{
	std::ifstream ifs("/proc/cpuinfo");
	if ( not ifs.is_open() )
	{
		PWTRACE("failed to open /proc/cpuinfo");
		return false;
	}

	std::string line, key, value;
	const char* ptr;
	cpu_type cpu;
	cpu_cont tmp;
	while ( std::getline(ifs, line).good() )
	{
		if ( PWStr::trim(line).empty() )
		{
			//PWTRACE("insert!: %zu %f", cpu.index, cpu.freq);
			tmp.push_back(cpu);
			cpu.clear();
			continue;
		}

		if ( nullptr == (ptr = strchr(line.c_str(), ':')) ) return false;

		key.assign(line.c_str(), ptr - line.c_str());
		++ptr;
		value.assign(ptr);

		PWStr::trim(key);
		PWStr::trim(value);

		//PWTRACE("%s:%s", key.c_str(), value.c_str());

		if ( 0 == strcasecmp(key.c_str(), "processor") ) cpu.index = atoi(value.c_str());
		else if ( 0 == strcasecmp(key.c_str(), "cpu MHz") ) cpu.freq = strtod(value.c_str(), nullptr);
	}

	out.swap(tmp);

	return true;
}

bool
SystemInformation::s_getNIC(nic_cont& out)
{
	int sfd(socket(PF_INET, SOCK_STREAM, 0));
	if ( -1 == sfd ) return false;

	nic_cont tmp;

	struct ifaddrs* ifap;
	::getifaddrs(&ifap);
	struct ifaddrs* ptr(ifap);

	struct ifreq ifr;

	while ( ptr )
	{
		nic_type nic;
		nic.name = ptr->ifa_name;
		nic.index = if_nametoindex(nic.name.c_str());

		const short int flags(ptr->ifa_flags);
		nic.flags.up = flags bitand IFF_UP;
		nic.flags.broadcast = flags bitand IFF_BROADCAST;
		nic.flags.debug = flags bitand IFF_DEBUG;
		nic.flags.loopback = flags bitand IFF_LOOPBACK;
		nic.flags.p2p = flags bitand IFF_POINTOPOINT;
		nic.flags.no_trailers = flags bitand IFF_NOTRAILERS;
		nic.flags.running = flags bitand IFF_RUNNING;
		nic.flags.no_arp = flags bitand IFF_NOARP;
		nic.flags.promisc = flags bitand IFF_PROMISC;

		strcpy(ifr.ifr_name, nic.name.c_str());
		if ( 0 == ioctl(sfd, SIOCGIFMETRIC, &ifr) ) nic.metric = ifr.ifr_metric;
		if ( 0 == ioctl(sfd, SIOCGIFMTU, &ifr) ) nic.mtu = ifr.ifr_mtu;
		
#ifdef SIOCGIFHWADDR
		// Get MAC address!
		if ( 0 == ioctl(sfd, SIOCGIFHWADDR, &ifr) )
		{
			const unsigned char* hwptr((unsigned char*) &ifr.ifr_hwaddr.sa_data);
			std::stringstream ss;
			for ( size_t i(0); i < 5; i++ )
			{
				ss.setf(std::ios::hex, std::ios::basefield);
				ss.fill('0');
				ss.width(2);
				ss << (int)hwptr[i];
				ss.setf(std::ios::dec, std::ios::basefield);
				ss.width(1);
				ss << ':';
			}

			ss.setf(std::ios::hex, std::ios::basefield);
			ss.fill('0');
			ss.width(2);
			ss << (int)hwptr[5];

			ss.str().swap(nic.hwaddr);
		}
#endif//SIOCGIFHWADDR

		if ( ptr->ifa_addr )
		{
			SocketAddress sa(ptr->ifa_addr, sizeof(struct sockaddr));
#ifdef PF_PACKET
			if ( sa.getFamily() == PF_PACKET )
			{
				ptr = ptr->ifa_next;
				continue;
			}
#endif//PF_PACKET

			char name[1024];
			if ( sa.getName(name, sizeof(name), nullptr, 0, nullptr) ) nic.addr = name;
		}

		if ( ptr->ifa_netmask )
		{
			if ( ptr->ifa_netmask )
			{
				SocketAddress sa(ptr->ifa_netmask, sizeof(struct sockaddr));
				char name[1024];
				if ( sa.getName(name, sizeof(name), nullptr, 0, nullptr) ) nic.mask = name;
			}
		}

		if ( nic.flags.broadcast )
		{
			if ( ptr->ifa_broadaddr )
			{
				SocketAddress sa(ptr->ifa_broadaddr, sizeof(struct sockaddr));
				char name[1024];
				if ( sa.getName(name, sizeof(name), nullptr, 0, nullptr) ) nic.baddr = name;
			}
		}

		tmp.push_back(nic);
		ptr = ptr->ifa_next;
	}

	::freeifaddrs(ifap);
	::close(sfd);

	out.swap(tmp);

	return true;
}

bool
SystemInformation::s_getMemory(memory_type& out)
{
	FILE* fp(::fopen("/proc/meminfo", "r"));
	if ( nullptr == fp ) return false;


	do {
		size_t tmp;

		if ( ::fscanf(fp, "%*s %zu %*s\n", &out.mem.total) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &out.mem.free) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &tmp) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &out.swap.total) < 1 ) break;
		if ( ::fscanf(fp, "%*s %zu %*s\n", &out.swap.free) < 1 ) break;

		out.mem.total /= 1024;
		out.mem.free /= 1024;
		out.swap.total /= 1024;
		out.swap.free /= 1024;

		::fclose(fp);

		return true;
	} while (false);

	return false;
}

bool
SystemInformation::s_getFileSystem(fs_type& out, const char* mp)
{
	struct statvfs vfs;
	if ( -1 == ::statvfs(mp, &vfs) ) return false;

	double total((double)vfs.f_blocks/1024.0/1024.0);
	double free((double)vfs.f_bavail/1024.0/1024.0);

	total *= vfs.f_bsize;
	free *= vfs.f_bsize;

	out.total = (size_t)total;
	out.free = (size_t)free;
	out.name = mp;

	return true;
}

bool
SystemInformation::s_getFileSystem(fs_cont& out)
{
#if defined(OS_LINUX)
	FILE* fp(setmntent(_PATH_MOUNTED, "r"));
	if ( nullptr == fp ) return false;

	fs_cont tmp;
	fs_type fs;

	struct mntent mntbuf;
	char buf[1024+1];

	while ( nullptr not_eq getmntent_r(fp, &mntbuf, buf, sizeof(buf)) )
	{
		if ( not s_getFileSystem(fs, mntbuf.mnt_dir) )
		{
			::fclose(fp);
			return false;
		}

		tmp.insert(fs_cont::value_type(mntbuf.mnt_dir, fs));
	}

	::fclose(fp);
	out.swap(tmp);

	return true;
#elif defined(OS_BSD) || defined(OS_APPLE)
	auto fstab = getfsent();
	if ( nullptr == fstab ) return false;
	
	fs_cont tmp;
	fs_type fs;
	while ( fstab )
	{
		if ( not s_getFileSystem(fs, fstab->fs_file) )
		{
			endfsent();
			return false;
		}
		
		tmp.insert(fs_cont::value_type(fstab->fs_file, fs));
		++fstab;
	}
	
	endfsent();
	out.swap(tmp);
	
	return true;
#endif
}

bool
SystemInformation::getAll(void)
{
	if ( not s_getOS(m_os) ) return false;
	if ( not s_getCPU(m_cpu) ) return false;
	if ( not s_getMemory(m_memory) ) return false;
	if ( not s_getNIC(m_nic) ) return false;
	if ( not s_getFileSystem(m_fs) ) return false;

	return true;
}

};//namespace pw
