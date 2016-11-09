project(aifil-utils)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(AIFIL_UTILS_TOP_LEVEL "ON")

if(WIN32)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
endif(WIN32)

find_package(Boost COMPONENTS filesystem system date_time QUIET)
if (${Boost_FOUND})
	include_directories(${Boost_INCLUDE_DIRS})
	add_definitions(-DHAVE_BOOST)
	message(STATUS "Boost: found ${Boost_LIB_VERSION}")
	set(AIFIL_UTILS_COMMON_DEPS ${Boost_LIBRARIES})
else()
	message(WARNING "Boost is not found. Some file utilities will be unavailable!")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_subdirectory(common)
add_subdirectory(imgproc)

if (${Boost_FOUND})
	add_subdirectory(video)
else()
	message(WARNING "Boost is required for building 'video' module.\
Now this module is disabled. If you need it, please firstly install boost library.")
endif()

get_directory_property(HAVE_PARENT PARENT_DIRECTORY)
if(HAVE_PARENT)
	set(AIFIL_UTILS_COMMON_DEPS ${AIFIL_UTILS_COMMON_DEPS} PARENT_SCOPE)
	set(AIFIL_UTILS_IMGPROC_DEPS ${AIFIL_UTILS_IMGPROC_DEPS} PARENT_SCOPE)
endif()

# add_library(aifil-utils-all ${OBJ_UTILS})
