cmake_minimum_required(VERSION 3.0)
project(__PWTEMPLATE_NAME__ CXX)
include(FindPkgConfig)

pkg_search_module(PW REQUIRED pw)

set(TARGET ${PROJECT_NAME})

add_executable(${TARGET}
	main.cpp
	mycommon.cpp
	myinstance.cpp
)

add_definitions(${PW_CFLAGS})

target_link_libraries(${TARGET} ${PW_LIBRARIES})