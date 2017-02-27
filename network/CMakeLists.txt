project(aifil-utils-network)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(NETCLIENT_FILES
    netclient.h
    netclient.cpp
)

set(NETSERVER_FILES
	netserver.hpp
    netserver.cpp
)

add_library(aifil-utils-network ${NETCLIENT_FILES} ${NETSERVER_FILES})
