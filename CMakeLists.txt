project(aifil-utils)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(AIFIL_UTILS_TOP_LEVEL "ON")

if(WIN32)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
endif(WIN32)

add_subdirectory(common)
add_subdirectory(imgproc)
add_subdirectory(video)

# add_library(aifil-utils-all ${OBJ_UTILS})
