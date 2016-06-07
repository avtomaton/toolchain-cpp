project(aifil-utils-imgproc)

cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFile)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

find_package(FFMPEG QUIET)
include_directories(${FFMPEG_INCLUDE_DIR})
if (FFMPEG_FOUND)
	add_definitions(-DHAVE_SWSCALE)
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR})

set(OBJ_UTILS
	filter.cpp
	filter.h
	image-grid.cpp
	image-grid.h
	imgproc.cpp
	imgproc.h
	mat-cache.cpp
	mat-cache.h
	pixel.h
	# scales-handler.cpp
	# scales-handler.h
)

add_library(aifil-utils-imgproc ${OBJ_UTILS})
