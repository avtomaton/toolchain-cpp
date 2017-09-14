include(CheckIncludeFileCXX)
get_directory_property(HAS_PARENT PARENT_DIRECTORY)

if (NOT "${CMAKE_PREFIX_PATH}" STREQUAL "")
	list(APPEND EXT_CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH})
elseif (NOT "$ENV{CMAKE_PREFIX_PATH}" STREQUAL "")
	list(APPEND EXT_CMAKE_ARGS -DCMAKE_PREFIX_PATH=$ENV{CMAKE_PREFIX_PATH})
endif()
if (DEFINED IOS_BUILD)
	list(APPEND EXT_CMAKE_ARGS -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS})
	list(APPEND EXT_CMAKE_ARGS -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS})
	list(APPEND EXT_CMAKE_ARGS -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS})
	list(APPEND EXT_CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
	list(APPEND EXT_CMAKE_ARGS -DIOS_BUILD=TRUE)
endif()

if (NOT DEFINED CLANDMARK_DIR)
	set(CLANDMARK_DIR ${REPO_DIR}/clandmark)
endif()
if (TARGET clandmark OR TARGET clandmark-lib)
	message(STATUS "Clandmark was already added")
	include_directories(${CLANDMARK_DIR}/libclandmark)
elseif (EXISTS "${CLANDMARK_DIR}/CMakeLists.txt")
	message(STATUS "Using local clandmark")
	add_subdirectory(
		${CLANDMARK_DIR}
		clandmark
	)
	include_directories(${CLANDMARK_DIR}/libclandmark)
else()
	message(STATUS
		"admobilize-clandmark is not found in ${CLANDMARK_DIR}, "
		"trying to clone from aifil git... "
		"If this won't work, try to explicitly setup CLANDMARK_DIR or "
		"properly set REPO_DIR to directory containing clandmark")
	ExternalProject_Add(
		clandmark-lib
		PREFIX clandmark
		GIT_REPOSITORY "git@github.com:avtomaton/clandmark.git"
		GIT_TAG master
		INSTALL_COMMAND ""
		CMAKE_ARGS ${EXT_CMAKE_ARGS}
	)
	ExternalProject_Get_property(clandmark-lib source_dir)
	include_directories(${source_dir}/libclandmark)
	set(CLANDMARK_DIR ${source_dir})
	if (HAS_PARENT)
		set(CLANDMARK_DIR ${source_dir} PARENT_SCOPE)
	endif()
	ExternalProject_Get_property(clandmark-lib binary_dir)
	link_directories(${binary_dir})
	set(EXTERNAL_CLANDMARK TRUE)
	if (HAS_PARENT)
		set(EXTERNAL_CLANDMARK TRUE PARENT_SCOPE)
	endif()
endif()

check_include_file_cxx(rapidxml.hpp HAVE_RAPIDXML)
if (NOT HAVE_RAPIDXML)
	include_directories(${CLANDMARK_DIR}/3rd_party/rapidxml-1.13)
endif()

