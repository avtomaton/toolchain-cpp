if (APPLE)
	include_directories(/usr/local/opt/openblas/include)
	link_directories(/usr/local/opt/openblas/lib)
endif()

find_package(Caffe QUIET)
if (${Caffe_FOUND})
	message(STATUS "Caffe: found local")
	add_definitions(-DHAVE_CAFFE)
	add_definitions(${Caffe_DEFINITIONS})
	include_directories(${Caffe_INCLUDE_DIRS})
elseif(NOT TARGET caffe-lib)
	message("Installing Caffe. "
			"Boost, Protobuf, MKL/ATLAS/openblas, hdf5 and lmdb are required! "
			"Downloading from git...")
	if (APPLE)
		message("You have to uninstall your openblas and "
				"install it with 'brew install --fresh -vd openblas'")
		set(EXT_ARGS
			-DBUILD_python=OFF
			-DUSE_LEVELDB=OFF
			-DBUILD_docs=OFF
			-DCMAKE_CXX_FLAGS=-I/usr/local/opt/openblas/include
			)
	else()
		# stupid workaround for external project weird configuration steps order
		message("RUN cmake one more time to build project properly, "
				"now only building dependencies!")
		set(EXT_ARGS
			-DBUILD_python=OFF
			-DUSE_LEVELDB=OFF
			-DBUILD_docs=OFF)
	endif()
	ExternalProject_Add(
		caffe-lib
		PREFIX caffe
		GIT_REPOSITORY https://github.com/BVLC/caffe.git
		GIT_TAG master
		CMAKE_ARGS ${EXT_ARGS})
	ExternalProject_Get_property(caffe-lib binary_dir)
	link_directories(${binary_dir}/lib)
	set(EXTERNAL_CAFFE TRUE)
	add_definitions(-DHAVE_CAFFE)
endif()
