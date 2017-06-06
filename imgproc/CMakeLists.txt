project(aifil-utils-imgproc)

cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFile)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

set(AIFIL_UTILS_IMGPROC_DEPS ${OpenCV_LIBS})

find_package(FFMPEG QUIET)
if (FFMPEG_FOUND)
	include_directories(${FFMPEG_INCLUDE_DIR})
	add_definitions(-DHAVE_SWSCALE)
	list(APPEND AIFIL_UTILS_IMGPROC_DEPS ${FFMPEG_LIBS})
endif()

set(OBJ_UTILS
	filter.cpp
	filter.hpp
	image-grid.cpp
	image-grid.hpp
	imgproc.cpp
	imgproc.hpp
	mat-cache.cpp
	mat-cache.hpp
	pixel.hpp
	math-helpers.cpp
	# scales-handler.cpp
	# scales-handler.hpp
)

add_library(aifil-utils-imgproc ${OBJ_UTILS})
set(AIFIL_UTILS_IMGPROC_DEPS ${AIFIL_UTILS_IMGPROC_DEPS} PARENT_SCOPE)
