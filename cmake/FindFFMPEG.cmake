#.rst:
# FindFFMPEG
# ----------
#
# Find the native FFMPEG includes and library
#
# This module defines::
#
#  FFMPEG_INCLUDE_DIR, where to find avcodec.h, avformat.h ...
#  FFMPEG_LIBRARIES, the libraries to link against to use FFMPEG.
#  FFMPEG_FOUND, If false, do not try to use FFMPEG.
#
# also defined, but not for general use are::
#
#   FFMPEG_AVFORMAT_LIBRARY, where to find the FFMPEG avformat library.
#   FFMPEG_AVCODEC_LIBRARY, where to find the FFMPEG avcodec library.
#
# This is useful to do it this way so that we can always add more libraries
# if needed to ``FFMPEG_LIBRARIES`` if ffmpeg ever changes...

#=============================================================================
# Copyright: 1993-2008 Ken Martin, Will Schroeder, Bill Lorensen
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of YCM, substitute the full
#  License text for the above reference.)

# Originally from VTK project


find_path(FFMPEG_AVFORMAT_INCLUDE_DIR libavformat/avformat.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
  $ENV{FFMPEG_DIR}/libavformat
  $ENV{FFMPEG_DIR}/include
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
  /usr/include/libavformat
  /usr/include/ffmpeg/libavformat
  /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}/libavformat
  /usr/local/include/libavformat
)

find_path(FFMPEG_AVUTIL_INCLUDE_DIR libavutil/avutil.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
  $ENV{FFMPEG_DIR}/libavutil
  $ENV{FFMPEG_DIR}/include
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
  /usr/include/libavutil
  /usr/include/ffmpeg/libavutil
  /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}/libavutil
  /usr/local/include/libavutil
)

find_path(FFMPEG_AVCODEC_INCLUDE_DIR libavcodec/avcodec.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
  $ENV{FFMPEG_DIR}/libavcodec
  $ENV{FFMPEG_DIR}/include
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
  /usr/include/libavcodec
  /usr/include/ffmpeg/libavcodec
  /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}/libavcodec
  /usr/local/include/libavcodec
)

find_path(FFMPEG_SWSCALE_INCLUDE_DIR libswscale/swscale.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
  $ENV{FFMPEG_DIR}/libswscale
  $ENV{FFMPEG_DIR}/include
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
  /usr/include/libswscale
  /usr/include/ffmpeg/libswscale
  /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}/libswscale
  /usr/local/include/libswscale
)

find_path(FFMPEG_AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/ffmpeg
  $ENV{FFMPEG_DIR}/libavdevice
  $ENV{FFMPEG_DIR}/include
  $ENV{FFMPEG_DIR}/include/ffmpeg
  /usr/local/include/ffmpeg
  /usr/include/ffmpeg
  /usr/include/libavdevice
  /usr/include/ffmpeg/libavdevice
  /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}/libavdevice
  /usr/local/include/libavdevice
)

if(FFMPEG_AVFORMAT_INCLUDE_DIR)
  if(FFMPEG_AVUTIL_INCLUDE_DIR)
    if(FFMPEG_AVCODEC_INCLUDE_DIR)
      set(FFMPEG_INCLUDE_DIR ${FFMPEG_AVFORMAT_INCLUDE_DIR}
                             ${FFMPEG_AVUTIL_INCLUDE_DIR}
                             ${FFMPEG_AVCODEC_INCLUDE_DIR})
    endif()
  endif()
endif()

if(FFMPEG_SWSCALE_INCLUDE_DIR)
  set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR}
                         ${FFMPEG_SWSCALE_INCLUDE_DIR})
endif()

if(FFMPEG_AVDEVICE_INCLUDE_DIR)
  set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR}
                         ${FFMPEG_AVDEVICE_INCLUDE_DIR})
endif()

find_library(FFMPEG_AVFORMAT_LIBRARY avformat
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib
  $ENV{FFMPEG_DIR}/libavformat
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_AVCODEC_LIBRARY avcodec
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib
  $ENV{FFMPEG_DIR}/libavcodec
  /usr/local/lib
  /usr/lib
)

find_library(FFMPEG_AVUTIL_LIBRARY avutil
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib
  $ENV{FFMPEG_DIR}/libavutil
  /usr/local/lib
  /usr/lib
)

if(NOT DISABLE_SWSCALE)
  find_library(FFMPEG_SWSCALE_LIBRARY swscale
    $ENV{FFMPEG_DIR}
    $ENV{FFMPEG_DIR}/lib
    $ENV{FFMPEG_DIR}/libswscale
    /usr/local/lib
    /usr/lib
  )
endif(NOT DISABLE_SWSCALE)

find_library(FFMPEG_AVDEVICE_LIBRARY avdevice
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib
  $ENV{FFMPEG_DIR}/libavdevice
  /usr/local/lib
  /usr/lib
)

find_library(_FFMPEG_z_LIBRARY_ z
  $ENV{FFMPEG_DIR}
  $ENV{FFMPEG_DIR}/lib
  /usr/local/lib
  /usr/lib
)



if(FFMPEG_INCLUDE_DIR)
  if(FFMPEG_AVFORMAT_LIBRARY)
    if(FFMPEG_AVCODEC_LIBRARY)
      if(FFMPEG_AVUTIL_LIBRARY)
        set(FFMPEG_FOUND "YES")
        set(FFMPEG_LIBRARIES ${FFMPEG_AVFORMAT_LIBRARY}
                             ${FFMPEG_AVCODEC_LIBRARY}
                             ${FFMPEG_AVUTIL_LIBRARY}
          )
        if(FFMPEG_SWSCALE_LIBRARY)
          set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES}
                               ${FFMPEG_SWSCALE_LIBRARY}
          )
        endif()
        if(FFMPEG_AVDEVICE_LIBRARY)
          set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES}
                               ${FFMPEG_AVDEVICE_LIBRARY}
          )
        endif()
        if(_FFMPEG_z_LIBRARY_)
          set( FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES}
                                ${_FFMPEG_z_LIBRARY_}
          )
        endif()
      endif()
    endif()
  endif()
endif()

if(FFMPEG_FOUND)
  set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR}
                         $ENV{FFMPEG_DIR}/include
      )
endif()
  

mark_as_advanced(
  FFMPEG_INCLUDE_DIR
  FFMPEG_AVFORMAT_INCLUDE_DIR
  FFMPEG_AVUTIL_INCLUDE_DIR
  FFMPEG_AVCODEC_INCLUDE_DIR
  FFMPEG_SWSCALE_INCLUDE_DIR
  FFMPEG_AVDEVICE_INCLUDE_DIR
  FFMPEG_AVFORMAT_LIBRARY
  FFMPEG_AVCODEC_LIBRARY
  FFMPEG_AVUTIL_LIBRARY
  FFMPEG_SWSCALE_LIBRARY
  FFMPEG_AVDEVICE_LIBRARY
  _FFMPEG_z_LIBRARY_
  )

# Set package properties if FeatureSummary was included
if(COMMAND set_package_properties)
  set_package_properties(FFMPEG PROPERTIES DESCRIPTION "A complete, cross-platform solution to record, convert and stream audio and video")
  set_package_properties(FFMPEG PROPERTIES URL "http://ffmpeg.org/")
endif()