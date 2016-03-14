project(aifil-utils)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(WIN32)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
endif(WIN32)

SET(OBJ_UTILS
cached-vector.cpp
cached-vector.h
conf-parser.cpp
conf-parser.h
console.cpp
console.h
errutils.cpp
errutils.h
fileutils.cpp
fileutils.h
logging.cpp
logging.h
profiler.cpp
profiler.h
stringutils.cpp
stringutils.h
timeutils.cpp
timeutils.h
)

add_library(aifilutils ${OBJ_UTILS})
