project(aifil-utils-video)

cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFile)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(FFMPEG REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIR})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(OBJ_UTILS
	media-reader.hpp
	media-reader.cpp
	media-writer.hpp
	media-writer.cpp
)

add_library(aifil-utils-video ${OBJ_UTILS})
