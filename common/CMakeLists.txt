project(aifil-utils-common)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

find_package(OpenCV QUIET)
if (${OpenCV_FOUND})
	include_directories(${OpenCV_INCLUDE_DIRS})
	add_definitions(-DHAVE_OPENCV)
	message(STATUS "OpenCV: found ${OpenCV_VERSION}")
endif()

set(OBJ_UTILS
	cached-vector.cpp
	conf-parser.cpp
	console.cpp
	errutils.cpp
	fileutils.cpp
	logging.cpp
	profiler.cpp
	rngutils.cpp
	stringutils.cpp
	timeutils.cpp
	c_attribs.hpp
	adjacency-matrix.cpp adjacency-matrix.h
	typedefs.hpp)

add_library(aifil-utils-common ${OBJ_UTILS})

