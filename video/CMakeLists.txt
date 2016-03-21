project(aifil-utils-video)

cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFile)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

check_include_file(libavcodec/avcodec.h HAVE_AVCODEC)
check_include_file(libavformat/avformat.h HAVE_AVFORMAT)

if (NOT HAVE_AVCODEC OR NOT HAVE_AVFORMAT)
	message(FATAL_ERROR "You should have avcodec and avformat installed")
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(OBJ_UTILS
	media-reader.h
	media-reader.cpp
)
include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR})

add_library(aifil-utils-video ${OBJ_UTILS})
