# The MIT License (MIT)
# Copyright (c) 2015 SK PLANET. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# \file CMakeLists.txt
# \brief CMake Build script for pw library for system check.
# \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
# \license This project is released under the MIT License.

enable_language(C CXX)
include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(CheckStructHasMember)
include(TestBigEndian)
include(FindPkgConfig)

set(PW_ADDITIONAL_LIBS "")
set(PW_ADDITIONAL_INC_DIRS "")

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
	set(LINUX 1)
endif ()

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" )
	set(FREEBSD 1)
	set(BSD 1)
endif ()

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" )
	set(OPENBSD 1)
	set(BSD 1)
endif ()

if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" )
	set(APPLE 1)
endif ()

if (UNIX)
	set(OS_UNIX 1)
endif()

if (LINUX)
	set(OS_LINUX 1)
endif()

if (BSD)
	set(OS_BSD 1)
endif()

if (APPLE)
	set(OS_APPLE 1 CACHE BOOL "Apple MacOS")
endif()

test_big_endian(WORDS_BIGENDIAN)

# Check C++11
set(CHECK_CXX11 "#include <unordered_map>
#include <algorithm>
int main() {
	std::unordered_map<int, int> cont;
	std::find_if(cont.begin(), cont.end(), [] (decltype(*cont.begin()) x) {
		return false;
	});
	for ( auto& i : cont ) { }
	return 0;
}" )
set(CHECK_TIMED_MUTEX "#include <mutex>
int main() { std::timed_mutex lock; return 0; }")
set(CHECK_CXX11_TLS "#include <thread>
thread_local int g_int = 0;
int main() { int l_int = g_int; return 0; }")
set(CHECK_GNU_TLS "#include <thread>
__thread int g_int = 0;
int main() { int l_int = g_int; return 0; }")
set(CHECK_CXX11_LAMBDA "#include <unordered_map>
#include <algorithm>
std::unordered_map<int, int> cont;
int main() { std::find_if(cont.begin(), cont.end(), [] (decltype(*cont.begin()) x) { return false; }); }")

set(save_flags ${CMAKE_REQUIRED_FLAGS})
set(save_defs ${CMAKE_REQUIRED_DEFINITIONS})
set(save_libs ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_FLAGS -std=c++11 -pthread)
set(CMAKE_REQUIRED_DEFINITIONS -D_REENTRANT)
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_cxx_source_compiles("${CHECK_CXX11}" HAVE_CXX11)
check_cxx_source_compiles("${CHECK_TIMED_MUTEX}" HAVE_TIMED_MUTEX)
check_cxx_source_compiles("${CHECK_CXX11_TLS}" HAVE_CXX11_TLS)
check_cxx_source_compiles("${CHECK_GNU_TLS}" HAVE_GNU_TLS)
set(CMAKE_REQUIRED_FLAGS ${save_flags})
set(CMAKE_REQUIRED_DEFINITIONS ${save_defs})
set(CMAKE_REQUIRED_LIBRARIES ${save_libs})

if( NOT "${HAVE_CXX11}" )
	message(FATAL_ERROR "PW library needs C++11 standard.")
endif()

if( NOT "${HAVE_CXX_TLS}" AND NOT "${HAVE_GNU_TLS}" )
	message(FATAL_ERROR "PW library needs TLS.")
endif()

if (HAVE_CXX11)
	set(CXXFLAGS_STD "-std=c++11")
	set(CXXFLAGS_THREAD "-D_REENTRANT -pthread")
else()
	message(FATAL_ERROR "libpw needs C++11 standard")
endif()

# Check something...
check_include_file("arpa/inet.h" HAVE_ARPA_INET_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("limits.h" HAVE_LIMITS_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("mntent.h" HAVE_MNTENT_H)
check_include_file("ndir.h" HAVE_NDIR_H)
check_include_file("netdb.h" HAVE_NETDB_H)
check_include_file("netinet/in.h" HAVE_NETINET_IN_H)
check_include_file("nlist.h" HAVE_NLIST_H)
check_include_file("poll.h" HAVE_POLL_H)
check_include_file("pthread.h" HAVE_PTHREAD_H)
check_include_file("stdbool.h" HAVE_STDBOOL_H)
check_include_file("stddef.h" HAVE_STDDEF_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("sys/file.h" HAVE_SYS_FILE_H)
check_include_file("sys/filio.h" HAVE_SYS_FILIO_H)
check_include_file("sys/ioctl.h" HAVE_SYS_IOCTL_H)
check_include_file("sys/mount.h" HAVE_SYS_MOUNT_H)
check_include_file("sys/ndir.h" HAVE_SYS_NDIR_H)
check_include_file("sys/select.h" HAVE_SYS_SELECT_H)
check_include_file("sys/socket.h" HAVE_SYS_SOCKET_H)
check_include_file("sys/sockio.h" HAVE_SYS_SOCKIO_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("sys/statvfs.h" HAVE_SYS_STATVFS_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/vfs.h" HAVE_SYS_VFS_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("syslog.h" HAVE_SYSLOG_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("vfork.h" HAVE_VFORK_H)
check_include_file("ifaddrs.h" HAVE_IFADDRS_H)
check_include_file("net/if.h" HAVE_NET_IF_H)

check_type_size("char" SIZEOF_CHAR)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("short" SIZEOF_SHORT)
check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("time_t" SIZEOF_TIME_T)
check_type_size("struct timespec" HAVE_TIMESPEC_STRUCT)

set(CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
check_type_size("struct cmsghdr" HAVE_MSGHDR_MSG_CONTROL)

set(CMAKE_EXTRA_INCLUDE_FILES "netdb.h")
check_type_size("struct addrinfo" HAVE_ADDRINFO_STRUCT)

set(CMAKE_EXTRA_INCLUDE_FILES "netinet/in.h")
check_type_size("struct in_pktinfo" HAVE_IN_PKTINFO_STRUCT)
check_type_size("socklen_t" HAVE_SOCKLEN_T)

set(CMAKE_EXTRA_INCLUDE_FILES "net/if.h")
check_type_size("struct if_nameindex" HAVE_IF_NAMEINDEX_STRUCT)

set(CMAKE_EXTRA_INCLUDE_FILES "math.h")
set(CMAKE_REQUIRED_LIBRARIES "m")
check_function_exists("fmodl" HAVE_FMODL)

check_function_exists("alarm" HAVE_ALARM)
check_function_exists("atexit" HAVE_ATEXIT)
check_function_exists("bzero" HAVE_BZERO)
check_function_exists("des" HAVE_DES)
check_function_exists("dup2" HAVE_DUP2)
check_function_exists("epoll_wait" HAVE_EPOLL)
check_function_exists("epoll_create1" HAVE_EPOLL_CREATE1)
check_function_exists("fdatasync" HAVE_FDATASYNC)
check_function_exists("fork" HAVE_FORK)
check_function_exists("ftruncate" HAVE_FTRUNCATE)
check_function_exists("f_setauxfl" HAVE_F_SETAUXFL)
check_function_exists("f_setsig" HAVE_F_SETSIG)
check_function_exists("gethostbyaddr" HAVE_GETHOSTBYADDR)
check_function_exists("gethostbyaddr_r" HAVE_GETHOSTBYADDR_R)
check_function_exists("gethostbyname" HAVE_GETHOSTBYNAME)
check_function_exists("gethostbyname2_r" HAVE_GETHOSTBYNAME2_R)
check_function_exists("gethostbyname_r" HAVE_GETHOSTBYNAME_R)
check_function_exists("gethostname" HAVE_GETHOSTNAME)
check_function_exists("getlogin_r" HAVE_GETLOGIN_R)
check_function_exists("getmntent" HAVE_GETMNTENT)
check_function_exists("getpagesize" HAVE_GETPAGESIZE)
check_function_exists("getpwnam_r" HAVE_GETPWNAM_R)
check_function_exists("getpwuid_r" HAVE_GETPWUID_R)
check_function_exists("getservbyname_r" HAVE_GETSERVBYNAME_R)
check_function_exists("getservbyport_r" HAVE_GETSERVBYPORT_R)
check_function_exists("gettimeofday" HAVE_GETTIMEOFDAY)
check_function_exists("gmtime_r" HAVE_GMTIME_R)
check_function_exists("inet_aton" HAVE_INET_ATON)
check_function_exists("inet_ntoa" HAVE_INET_NTOA)
check_function_exists("isascii" HAVE_ISASCII)
check_function_exists("kqueue" HAVE_KQUEUE)
check_function_exists("libpthread" HAVE_LIBPTHREAD)
check_function_exists("libpthreads" HAVE_LIBPTHREADS)
check_function_exists("localtime_r" HAVE_LOCALTIME_R)
check_function_exists("malloc" HAVE_MALLOC)
check_function_exists("memchr" HAVE_MEMCHR)
check_function_exists("memmove" HAVE_MEMMOVE)
check_function_exists("memset" HAVE_MEMSET)
check_function_exists("mkstemp" HAVE_MKSTEMP)
check_function_exists("mmap" HAVE_MMAP)
check_function_exists("munmap" HAVE_MUNMAP)
check_function_exists("nlist_h" HAVE_NLIST_H)
check_function_exists("poll" HAVE_POLL)
check_function_exists("pselect" HAVE_PSELECT)
check_function_exists("putenv" HAVE_PUTENV)
check_function_exists("readdir_r" HAVE_READDIR_R)
check_function_exists("realloc" HAVE_REALLOC)
check_function_exists("select" HAVE_SELECT)
check_function_exists("socket" HAVE_SOCKET)
check_function_exists("stat_empty_string_bug" HAVE_STAT_EMPTY_STRING_BUG)
check_function_exists("strcasecmp" HAVE_STRCASECMP)
check_function_exists("strchr" HAVE_STRCHR)
check_function_exists("strdup" HAVE_STRDUP)
check_function_exists("strerror" HAVE_STRERROR)
check_function_exists("strftime" HAVE_STRFTIME)
check_function_exists("strncasecmp" HAVE_STRNCASECMP)
check_function_exists("strnlen" HAVE_STRNLEN)
check_function_exists("strrchr" HAVE_STRRCHR)
check_function_exists("strstr" HAVE_STRSTR)
check_function_exists("strtol" HAVE_STRTOL)
check_function_exists("strtoul" HAVE_STRTOUL)
check_function_exists("strtoull" HAVE_STRTOULL)
check_function_exists("tzset" HAVE_TZSET)
check_function_exists("uname" HAVE_UNAME)
check_function_exists("vfork" HAVE_VFORK)
check_function_exists("vprintf" HAVE_VPRINTF)
check_function_exists("working_fork" HAVE_WORKING_FORK)
check_function_exists("working_vfork" HAVE_WORKING_VFORK)
check_function_exists("_bool" HAVE__BOOL)
check_function_exists("_posix_getpwnam_r" HAVE__POSIX_GETPWNAM_R)
check_function_exists("_posix_getpwuid_r" HAVE__POSIX_GETPWUID_R)

check_function_exists("getaddrinfo" HAVE_GETADDRINFO_PROTO)
check_function_exists("getnameinfo" HAVE_GETNAMEINFO_PROTO)
check_function_exists("gethostname" HAVE_GETHOSTNAME_PROTO)
check_function_exists("hstrerror" HAVE_HSTRERROR_PROTO)
check_function_exists("if_nametoindex" HAVE_IF_NAMETOINDEX_PROTO)
check_function_exists("inet_pton" HAVE_INET_PTON_PROTO)
check_function_exists("inet_aton" HAVE_INET_ATON_PROTO)
check_function_exists("isfdtype" HAVE_ISFDTYPE_PROTO)
check_function_exists("pselect" HAVE_PSELECT_PROTO)
check_function_exists("sockatmark" HAVE_SOCKATMARK_PROTO)
check_function_exists("snprintf" HAVE_SNPRINTF_PROTO)

check_function_exists("getifaddrs" HAVE_GETIFADDRS)
check_function_exists("if_nameindex" HAVE_IF_NAMEINDEX)

set(CMAKE_REQUIRED_LIBRARIES pthread)
check_function_exists("pthread_spin_timedlock" HAVE_PTHREAD_SPIN_TIMEDLOCK)
check_function_exists("pthread_rwlock_timedwrlock"
HAVE_PTHREAD_RWLOCK_TIMEDWRLOCK)

set(CMAKE_EXTRA_INCLUDE_FILES netinet/in.h;arpa/inet.h)
check_function_exists("htons" HAVE_HTONS)
check_function_exists("htonl" HAVE_HTONL)
check_function_exists("htonll" HAVE_HTONLL)

# Check int128_t
set(CHECK_INT128_T "int main(void) { int128_t i; }")
set(CHECK_INT128_T_GNU "int main(void) { __int128_t i; }")

check_c_source_compiles("${CHECK_INT128_T}" HAVE_INT128_T)
check_c_source_compiles("${CHECK_INT128_T_GNU}" HAVE_INT128_T_GNU)
if ( HAVE_INT128_T OR HAVE_INT128_T_GNU )
	set(PW_HAVE_INT128 16)
endif ()

set(Boost_USE_MULTITHREADED TRUE)
include(FindBoost)
find_package(Boost 1.54 QUIET OPTIONAL_COMPONENTS regex)
if (Boost_REGEX_FOUND)
	set(PW_USE_BOOST_REGEX 1 CACHE BOOL "Use boost regex")
	set(_PW_PC_BOOST_REGEX_LDFLAGS -lboost_regex)
	set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} boost_regex)
	set(PW_ADDITIONAL_INC_DIRS ${PW_ADDITIONAL_INC_DIRS} ${Boost_INCLUDE_DIRS})
endif()

pkg_search_module(URIPARSER REQUIRED QUIET uriparser liburiparser)
if (URIPARSER_FOUND)
	set(HAVE_URIPARSER 1)
	set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} uriparser)
	set(PW_ADDITIONAL_INC_DIRS ${PW_ADDITIONAL_INC_DIRS} ${URIPARSER_INCLUDE_DIRS})
endif()

pkg_search_module(JSONCPP QUIET jsoncpp libjsoncpp)
if (JSONCPP_FOUND)
	set(HAVE_JSONCPP 1)
	set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} jsoncpp)
	set(PW_ADDITIONAL_INC_DIRS ${PW_ADDITIONAL_INC_DIRS} ${JSONCPP_INCLUDE_DIRS})
endif()

pkg_search_module(SQLITE3 QUIET sqlite3 libsqlite3)
if (SQLITE3_FOUND)
	set(HAVE_SQLITE3 1)
	set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} sqlite3)
endif()

# Add Zlib
set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} z)

# Check OpenSSL
include(FindOpenSSL)
find_package(OpenSSL 1.0.0 REQUIRED)
set(CMAKE_EXTRA_INCLUDE_FILES "${OPENSSL_INCLUDE_DIR}/openssl/ssl.h")
set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_LIBRARIES})
check_function_exists("SSLv2_method" HAVE_SSLV2_METHOD)
check_function_exists("TLSv1_method" HAVE_TLSV1_METHOD)
check_function_exists("TLSv1_1_method" HAVE_TLSV1_1_METHOD)
check_function_exists("TLSv1_2_method" HAVE_TLSV1_2_METHOD)

set(CMAKE_EXTRA_INCLUDE_FILES "${OPENSSL_INCLUDE_DIR}/openssl/evp.h")
set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_LIBRARIES})
check_type_size("EVP_CIPHER_CTX" HAVE_EVP_CIPHER_CTX)
check_type_size("EVP_MD_CTX" HAVE_EVP_MD_CTX)
check_function_exists("EVP_md2" HAVE_EVP_MD2)
check_function_exists("EVP_PKEY_CTX_new_id" HAVE_EVP_PKEY_CTX_NEW_ID)
check_function_exists("EVP_PKEY_new_mac_key" HAVE_EVP_PKEY_NEW_MAC_KEY)
check_function_exists("EVP_DigestSignInit" HAVE_EVP_DIGESTSIGNINIT)
check_function_exists("EVP_DigestVerifyInit" HAVE_EVP_DIGESTVERIFYINIT)
check_function_exists("EVP_aes_256_ctr" HAVE_EVP_AES_256_CTR)
check_function_exists("EVP_aes_256_ccm" HAVE_EVP_AES_256_CCM)
check_function_exists("EVP_aes_256_gcm" HAVE_EVP_AES_256_GCM)
check_function_exists("EVP_aes_256_wrap" HAVE_EVP_AES_256_WRAP)
check_function_exists("EVP_aes_256_xts" HAVE_EVP_AES_256_XTS)
check_function_exists("EVP_aes_192_xts" HAVE_EVP_AES_192_XTS)
check_function_exists("EVP_aes_256_cbc_hmac_sha256" HAVE_EVP_AES_256_CBC_HMAC_SHA256)

set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} ${OPENSSL_LIBRARIES})
set(PW_ADDITIONAL_INC_DIRS ${PW_ADDITIONAL_INC_DIRS} ${OPENSSL_INCLUDE_DIR})

# Cache Addtional flags...
set(PW_ADDITIONAL_LIBS ${PW_ADDITIONAL_LIBS} CACHE STRING "Addtional libraries")
set(PW_ADDITIONAL_INC_DIRS ${PW_ADDITIONAL_INC_DIRS} CACHE STRING "Addtional include directories")

message(STATUS "Additional libraries: ${PW_ADDITIONAL_LIBS}")
message(STATUS "Additional include directories: ${PW_ADDITIONAL_INC_DIRS}")

find_program(PW_HAVE_RPM rpm)
if ( PW_HAVE_RPM )
  set(PW_HAVE_RPM ${PW_HAVE_RPM} CACHE BOOL "Have RPM")
endif ()

find_program(PW_HAVE_DPKG dpkg)
if ( PW_HAVE_DPKG )
  set(PW_HAVE_DPKG ${PW_HAVE_DPKG} CACHE BOOL "Have DPKG")
  execute_process(COMMAND dpkg --print-architecture
	  OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
	  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif ()

configure_file(pwlib_config.cmake.h
	"${pw_SOURCE_DIR}/pwlib_config.h" @ONLY)
configure_file(Doxyfile.cmake.in
	"${pw_BINARY_DIR}/Doxyfile" @ONLY)
configure_file(pw.cmake.pc
	"${pw_BINARY_DIR}/pw.pc" @ONLY)
configure_file(libpw.cmake.spec
	"${pw_BINARY_DIR}/libpw.spec" @ONLY)
	
if (PW_HAVE_DPKG)
  configure_file(control.cmake.in
	  "${pw_BINARY_DIR}/${PW_PKGNAME_DEB}/DEBIAN/control" @ONLY)
endif ()	  
