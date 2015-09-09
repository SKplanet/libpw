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
# \brief CMake Build script for pw library.
# \copyright Copyright (c) 2015, SK PLANET. All Rights Reserved.
# \license This project is released under the MIT License.

cmake_minimum_required(VERSION 3.0)
project(pw C CXX)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

#set(CMAKE_USE_RELATIVE_PATHS true)
include(GNUInstallDirs)

set(PKGCONFIG_CONF_PATH "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
#set(CMAKE_INSTALL_PREFIX "/usr")

set(pw_NAME "lib${PROJECT_NAME}")
set(pw_VENDOR "(c) SK PLANET")
set(pw_CONTACT "purewell@sk.com")
set(pw_VERSION_MAJOR 1)
set(pw_VERSION_MINOR 0)
set(pw_VERSION_PATCH 2)
if ( VERSION_COUNT )
	set(pw_VERSION_RELEASE ${VERSION_COUNT})
else ()
	set(pw_VERSION_RELEASE 1)
endif()
set(pw_VERSION "${pw_VERSION_MAJOR}.${pw_VERSION_MINOR}.${pw_VERSION_PATCH}")
set(pw_DESCRIPTION_SUMMARY "PW library")
set(pw_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../DESCRIPTION")
file(READ ${pw_DESCRIPTION_FILE} pw_DESCRIPTION_DATA)
set(pw_URL "http://github.com/skplanet/libpw/")
set(pw_LICENSE "MIT")
set(pw_LICENSE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../LICENSE")
file(READ ${pw_LICENSE_FILE} pw_LICENSE_DATA)
set(pw_GROUP "Development/Libraries/C and C++")
set(pw_CHANGELOG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../ChangeLog")
file(READ ${pw_CHANGELOG_FILE} pw_CHANGELOG_DATA)

set(RPMBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/rpmbuild)
set(RPMBUILDROOT_DIR ${RPMBUILD_DIR}/BUILDROOT)
set(RPMSOURCE_FILE "libpw-${pw_VERSION}.tgz")

set(PWCXXFLAGS -std=c++11 -D_REENTRANT -pthread  -Wall -fpermissive -fPIC
	CACHE STRING "PW library C++ flags" FORCE)

set(PW_PKGNAME_RPM "${pw_NAME}-${pw_VERSION}")
set(PW_PKGNAME_DEB "${pw_NAME}-${pw_VERSION}")

add_subdirectory(_check)

set(PWLIBS z ssl crypto dl pthread)
set(PWINC_DIRS ${PW_ADDITIONAL_INC_DIRS} CACHE STRING "PW include directories" FORCE)
set(PWLIBS_STATIC libpw.a ${PWLIBS} ${PW_ADDITIONAL_LIBS} CACHE STRING "PW(static) libraries" FORCE)
set(PWLIBS_SHARED libpw.so ${PWLIBS} ${PW_ADDITIONAL_LIBS} CACHE STRING "PW(shared) libraries" FORCE )
set(PWLIBS ${PWLIBS_STATIC} CACHE STRING "PW libraries" FORCE)

include_directories(${PWINC_DIRS})
add_definitions(${PWCXXFLAGS})

# Set sources
file(GLOB HDRS *.h)
list(REMOVE_ITEM HDRS pw_iopoller_epoll.h pw_iopoller_select.h)

set(SRCS_COMMON pw_common.cpp)
set(SRCS_SYSTEM pw_log.cpp pw_timer.cpp pw_sysinfo.cpp pw_module.cpp pw_exception.cpp)
set(SRCS_STRING pw_string.cpp pw_tokenizer.cpp pw_ini.cpp pw_encode.cpp
	pw_date.cpp pw_key.cpp pw_compress.cpp pw_region.cpp pw_uri.cpp
	pw_strfltr.cpp)
set(SRCS_NETWORK pw_iopoller.cpp pw_iopoller_select.cpp pw_iopoller_epoll.cpp
	pw_socket.cpp pw_iobuffer.cpp pw_sockaddr.cpp
	pw_packet_if.cpp pw_channel_if.cpp pw_listener_if.cpp
	pw_msgpacket.cpp pw_msgchannel.cpp
	pw_httppacket.cpp pw_httpchannel.cpp
	pw_iprange.cpp pw_iprange_type.cpp
	pw_apnschannel.cpp pw_apnspacket.cpp
	pw_redischannel.cpp pw_redispacket.cpp
	pw_simplechpool.cpp
	)
set(SRCS_CRYPTO pw_ssl.cpp pw_digest.cpp pw_crypto.cpp)
set(SRCS_INSTANCE pw_instance_if.cpp pw_jobmanager.cpp pw_multichannel_if.cpp pw_concurrentqueue_if.cpp)

set(SRCS_ALL ${SRCS_COMMON} ${SRCS_SYSTEM} ${SRCS_STRING} ${SRCS_NETWORK} ${SRCS_CRYPTO} ${SRCS_INSTANCE})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_LIBDIR})

add_library(pw_shared SHARED ${SRCS_ALL})
add_library(pw_static STATIC ${SRCS_ALL})

add_custom_target(doc
	COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

add_custom_target(libpw.rpm
	COMMAND rpmbuild -bb --define "_topdir ${RPMBUILD_DIR}" --buildroot="${RPMBUILDROOT_DIR}" ${CMAKE_CURRENT_BINARY_DIR}/libpw.spec
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Install files for RPM..."
)

add_custom_target(libpw.deb
	COMMAND make DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/${PW_PKGNAME_DEB} install
	COMMAND dpkg -b ${PW_PKGNAME_DEB}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Install files for DEB..."
)

set_target_properties(pw_shared
	PROPERTIES
		VERSION ${pw_VERSION}
		SOVERSION ${pw_VERSION_MAJOR}
		OUTPUT_NAME pw)

set_target_properties(pw_static
	PROPERTIES
		VERSION ${pw_VERSION}
		SOVERSION ${pw_VERSION_MAJOR}
		OUTPUT_NAME pw)

install(TARGETS pw_static pw_shared
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${HDRS}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pw/)

install(FILES
	${pw_LICENSE_FILE} ${pw_DESCRIPTION_FILE} ${pw_CHANGELOG_FILE}
	${libpw_top_SOURCE_DIR}/CONTRIBUTORS.md
	${libpw_top_SOURCE_DIR}/README.md
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/libpw)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/pw.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/)

include(CPack)

# Infomation messages...
message(STATUS "Include directories(PWINC_DIRS): ${PWINC_DIRS}")
message(STATUS "Libraries(PWLIBS): ${PWLIBS}")
message(STATUS "Libraries for static(PWLIBS_STATIC): ${PWLIBS_STATIC}")
message(STATUS "Libraries for shared(PWLIBS_SHARED): ${PWLIBS_SHARED}")
message(STATUS "Watch pw.pc(${CMAKE_CURRENT_BINARY_DIR}/pw.pc) for building flags")

# Installation test...
# add_custom_target(.instbin DESTDIR=${libpw_top_BINARY_DIR} make install 2>&1 > /dev/null
# 	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})