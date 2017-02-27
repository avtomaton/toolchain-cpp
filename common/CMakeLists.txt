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
	cached-vector.hpp
	conf-parser.cpp
	conf-parser.hpp
	console.cpp
	console.hpp
	errutils.cpp
	errutils.hpp
	fileutils.cpp
	fileutils.hpp
	logging.cpp
	logging.hpp
	math-helpers.cpp
	math-helpers.hpp
	profiler.cpp
	profiler.hpp
	stringutils.cpp
	stringutils.hpp
	timeutils.cpp
	timeutils.hpp
	c_attribs.hpp
	adjacency-matrix.cpp adjacency-matrix.h)

add_library(aifil-utils-common ${OBJ_UTILS})

