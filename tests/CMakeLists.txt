project(toolchain-tests)

cmake_minimum_required(VERSION 2.8)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

find_package(OpenCV QUIET)
if (${OpenCV_FOUND})
	include_directories(${OpenCV_INCLUDE_DIRS})
	add_definitions(-DHAVE_OPENCV)
	message(STATUS "OpenCV: found ${OpenCV_VERSION}")
endif()


add_executable(main main.cpp test-adjacency-matrix.h)
target_link_libraries(main aifil-utils-common
		${Boost_LIBRARIES}
		${GTEST_LIBRARY}
		${GTEST_MAIN_LIBRARY}
		${CMAKE_THREAD_LIBS_INIT}
		gflags)
