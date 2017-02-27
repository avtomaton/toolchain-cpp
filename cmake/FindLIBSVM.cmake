find_path(LIBSVM_INCLUDE_DIR svm.h
	$ENV{LIBSVM_DIR}
	$ENV{HOME}/libsvm
	/usr/local/include/libsvm
	/usr/local/include
	/usr/include/libsvm
	/usr/include
)

if(LIBSVM_INCLUDE_DIR)
	set(LIBSVM_INCLUDE_DIRS ${LIBSVM_INCLUDE_DIR})
endif()

#if libsvm/ exist, but library not found, run in libsvm-dir: ln -s libsvm.so.2 libsvm.so
find_library(LIBSVM_LIBRARY svm
	$ENV{LIBSVM_DIR}
	$ENV{HOME}/libsvm
	/usr/local/lib/libsvm
	/usr/local/lib
	/usr/lib/libsvm
	/usr/lib
)

if(LIBSVM_INCLUDE_DIR)
  if(LIBSVM_LIBRARY)
  	set(LIBSVM_FOUND "YES")
  	set(LIBSVM_LIB ${LIBSVM_LIBRARY})
  endif()
endif()

if (UNIX)
  list(APPEND LIBSVM_LIB "-lm" )
endif()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (LIBSVM DEFAULT_MSG 
  LIBSVM_LIBRARY
  LIBSVM_LIB
  LIBSVM_INCLUDE_DIR
  LIBSVM_INCLUDE_DIRS
)

mark_as_advanced(
	LIBSVM_INCLUDE_DIR
	LIBSVM_INCLUDE_DIRS 
	LIBSVM_LIBRARY
	LIBSVM_LIB
)