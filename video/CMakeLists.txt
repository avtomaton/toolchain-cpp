project(aifil-utils-video)

cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFile)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(FFMPEG REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIR})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

find_package(Boost COMPONENTS filesystem system REQUIRED)
if (${Boost_FOUND})
	include_directories(${Boost_INCLUDE_DIRS})
	add_definitions(-DHAVE_BOOST)
	message(STATUS "Boost: found ${Boost_LIB_VERSION}")
endif()

set(OBJ_UTILS
	media-reader.h
	media-reader.cpp
)
include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR})

add_library(aifil-utils-video ${OBJ_UTILS})
