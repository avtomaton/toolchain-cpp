project(aifil-utils-network)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(NETCLIENT_FILES
	netclient.h
	netclient.cpp)

set(NETSERVER_FILES
	netserver.hpp
	netserver.cpp)

set(COMMON_FILES
	http-common.hpp
	http-common.cpp
	asio-timer.hpp
	asio-timer.cpp
	net-connection.hpp
	net-connection.cpp
	network-connection.hpp
	network-connection.cpp
	tcp-server.hpp
	tcp-server.cpp)

add_library(aifil-utils-network ${NETCLIENT_FILES} ${NETSERVER_FILES} ${COMMON_FILES})
