project (libonrisc C)

if(NOT VSOR_ARCH)
	SET (VSOR_ARCH "armhf")
endif(NOT VSOR_ARCH)

#CPack stuff
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OnRISC Hardware API Library")
SET(CPACK_PACKAGE_VENDOR "VScom")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "5")
SET(CPACK_PACKAGE_VERSION_PATCH "7")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libsoc | libsoc2")
SET(CPACK_PACKAGE_CONTACT "service@visionsystems.de")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${VSOR_ARCH})
INCLUDE(CPack)

cmake_minimum_required(VERSION 2.6)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(PYTHON_WRAP "Build Python bindings" OFF)
option(LUA_WRAP "Build Lua bindings" OFF)
option(NODEJS_WRAP "Build Node.js bindings" OFF)
option(NO_UDEV "No udev bindings" OFF)

# handle dependencies
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBSOC REQUIRED libsoc)

include_directories(BEFORE
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/include
	${PC_LIBSOC_INCLUDE_DIRS})
link_directories(${PC_LIBSOC_LIBDIR} )

configure_file(${PROJECT_SOURCE_DIR}/include/onrisc.h.in
	       ${PROJECT_BINARY_DIR}/onrisc.h @ONLY)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)

set(SWIG_ONRISC_I ${PROJECT_SOURCE_DIR}/swig/onrisc.i)
set(LIBS
	${PC_LIBSOC_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	)
	
if(NO_UDEV)
	add_definitions(-DNO_UDEV)
else(NO_UDEV)
	pkg_check_modules(PC_LIBUDEV REQUIRED libudev)
	include_directories(BEFORE ${PC_LIBUDEV_INCLUDE_DIRS})
	list(APPEND LIBS ${PC_LIBUDEV_LIBRARIES})
endif(NO_UDEV)


# library
add_library(onrisc ${SRCS} ${PROJECT_SOURCE_DIR}/include/onrisc.h)
set_target_properties(onrisc PROPERTIES
	PUBLIC_HEADER "${PROJECT_BINARY_DIR}/onrisc.h"
	VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
	SOVERSION 1)
target_link_libraries(onrisc ${LIBS})

add_subdirectory(${CMAKE_SOURCE_DIR}/cli)
add_subdirectory(${CMAKE_SOURCE_DIR}/swig)

install(TARGETS onrisc
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include COMPONENT dev
	)

# SWIG: Node.js
if (NODEJS_WRAP)
	find_package(SWIG REQUIRED)
	include(${SWIG_USE_FILE})
	set(SWIG_NODEJS "${CMAKE_BINARY_DIR}/swig_nodejs")
	file(MAKE_DIRECTORY ${SWIG_NODEJS})

	# search for node-gyp
	find_program(NODE_GYP node-gyp)
	if(NODE_GYP-NOTFOUND)
		message(FATAL_ERROR "Cannot find node-gyp!")
	endif(NODE_GYP-NOTFOUND)

	include_directories(${CMAKE_CURRENT_SOURCE_DIR})

	set(CMAKE_SWIG_FLAGS "-threads")
	file(COPY
		${SRCS}
		"${CMAKE_SOURCE_DIR}/include/vssys.h"
		"${CMAKE_BINARY_DIR}/onrisc.h"
		"${SWIG_ONRISC_I}"
		"${CMAKE_SOURCE_DIR}/swig/binding.gyp"
		"${CMAKE_SOURCE_DIR}/swig/index.js"
		"${CMAKE_SOURCE_DIR}/swig/package.json"
		DESTINATION ${SWIG_NODEJS})

	add_custom_command(
		TARGET onrisc
		POST_BUILD
		COMMAND swig -c++ -javascript -node -outcurrentdir
		-I${CMAKE_SOURCE_DIR}/src ${SWIG_ONRISC_I}
		WORKING_DIRECTORY ${SWIG_NODEJS}
		COMMENT "Creating Node.js bindings: swig")
endif (NODEJS_WRAP)

# PkgConfig
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})

configure_file(${CMAKE_SOURCE_DIR}/libonrisc.pc.in ${CMAKE_BINARY_DIR}/libonrisc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libonrisc.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
