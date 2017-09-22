project (libonrisc C)

if(NOT VSOR_ARCH)
	SET (VSOR_ARCH "armhf")
endif(NOT VSOR_ARCH)

#CPack stuff
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OnRISC Hardware API Library")
SET(CPACK_PACKAGE_VENDOR "VScom")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "5")
SET(CPACK_PACKAGE_VERSION_PATCH "5")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libsoc | libsoc2")
SET(CPACK_PACKAGE_CONTACT "service@visionsystems.de")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${VSOR_ARCH})
INCLUDE(CPack)

cmake_minimum_required(VERSION 2.6)

# handle dependencies
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBSOC REQUIRED libsoc)
pkg_check_modules(PC_LIBUDEV REQUIRED libudev)

include_directories(BEFORE
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}/include
	${PC_LIBSOC_INCLUDE_DIRS}
	${PC_LIBUDEV_INCLUDE_DIRS})
link_directories(${PC_LIBSOC_LIBDIR} ${PC_LIBUDEV_LIBDIR})

configure_file(${PROJECT_SOURCE_DIR}/include/onrisc.h.in
	       ${PROJECT_BINARY_DIR}/onrisc.h @ONLY)
set(SRCS
	src/onrisc.c
	src/leds.c
	src/gpios.c
	src/dips.c
	src/system.c
	src/uarts.c
	src/devices.c)

set(SWIG_ONRISC_I ${PROJECT_SOURCE_DIR}/swig/onrisc.i)

# SWIG: Python
if (PYTHON_WRAP)
	find_package(SWIG REQUIRED)
	include(${SWIG_USE_FILE})

	find_package(PythonInterp REQUIRED)
	find_package(PythonLibs REQUIRED)
	include_directories(${PYTHON_INCLUDE_PATH})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR})

	set(CMAKE_SWIG_FLAGS "")

	swig_add_module(onrisc python ${SWIG_ONRISC_I} ${SRCS})
	swig_link_libraries(onrisc ${PYTHON_LIBRARIES} ${PC_LIBSOC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${PC_LIBUDEV_LIBRARIES})
endif (PYTHON_WRAP)

# SWIG: Lua
if (LUA_WRAP)
	find_package(SWIG REQUIRED)
	include(${SWIG_USE_FILE})

	find_package(Lua51 REQUIRED)
	include_directories(${LUA_INCLUDE_DIR})

 	include_directories(${CMAKE_CURRENT_SOURCE_DIR})

	set(CMAKE_SWIG_FLAGS "")

	swig_add_module(luaonrisc lua ${SWIG_ONRISC_I} ${SRCS})
	swig_link_libraries(luaonrisc ${PC_LIBSOC_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${PC_LIBUDEV_LIBRARIES} ${LUA_LIBRARY})
endif (LUA_WRAP)

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

	set(CMAKE_SWIG_FLAGS "")
	file(COPY
		${SRCS}
		"${CMAKE_SOURCE_DIR}/include/vssys.h"
		"${CMAKE_BINARY_DIR}/onrisc.h"
		"${SWIG_ONRISC_I}"
		"${CMAKE_SOURCE_DIR}/swig/binding.gyp"
		DESTINATION ${SWIG_NODEJS})
endif (NODEJS_WRAP)

# library
add_library(onrisc SHARED ${SRCS} ${PROJECT_SOURCE_DIR}/include/onrisc.h)
set_target_properties(onrisc PROPERTIES
	PUBLIC_HEADER "${PROJECT_BINARY_DIR}/onrisc.h"
	VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
	SOVERSION 1)
target_link_libraries(onrisc ${PC_LIBSOC_LIBRARIES} ${PC_LIBUDEV_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (NODEJS_WRAP)
	add_custom_command(
		TARGET onrisc
		POST_BUILD
		COMMAND swig -c++ -javascript -node -outcurrentdir
		-I${CMAKE_SOURCE_DIR}/src ${SWIG_ONRISC_I}
		WORKING_DIRECTORY ${SWIG_NODEJS}
		COMMENT "Creating Node.js bindings: swig")
	add_custom_command(
		TARGET onrisc
		POST_BUILD
		COMMAND node-gyp configure build
		WORKING_DIRECTORY ${SWIG_NODEJS}
		COMMENT "Creating Node.js bindings: node-gyp")
endif (NODEJS_WRAP)

# cli
add_executable(onrisctool ${PROJECT_SOURCE_DIR}/cli/onrisctool.c)
target_link_libraries(onrisctool onrisc)

# cli
add_executable(onriscdipd ${PROJECT_SOURCE_DIR}/cli/onriscdipd.c)
target_link_libraries(onriscdipd onrisc)

add_executable(onriscgpiod ${PROJECT_SOURCE_DIR}/cli/onriscgpiod.c)
target_link_libraries(onriscgpiod onrisc)

install(TARGETS onrisc 
	LIBRARY DESTINATION lib COMPONENT shlib
	PUBLIC_HEADER DESTINATION include COMPONENT dev
	)
install(TARGETS onrisctool RUNTIME DESTINATION bin)
install(TARGETS onriscdipd RUNTIME DESTINATION bin)
install(TARGETS onriscgpiod RUNTIME DESTINATION bin)

# PkgConfig
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})

configure_file(${CMAKE_SOURCE_DIR}/libonrisc.pc.in ${CMAKE_BINARY_DIR}/libonrisc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libonrisc.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)

if (PYTHON_WRAP)
	execute_process (
	   COMMAND ${PYTHON_EXECUTABLE} -c
		 "import site, sys; sys.stdout.write(site.PREFIXES[-1])"
	   OUTPUT_VARIABLE PYTHON_PREFIX
	)
	 file ( TO_CMAKE_PATH "${PYTHON_PREFIX}" PYTHON_PREFIX )
	 execute_process (
	   COMMAND ${PYTHON_EXECUTABLE} -c
		 "import site, sys; sys.stdout.write(site.getsitepackages()[-1])"
	   OUTPUT_VARIABLE PYTHON_SITE_DIR
	 )
	file ( TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR )
	 string ( REGEX REPLACE "^${PYTHON_PREFIX}/" ""
	   PYTHON_SITE_DIR "${PYTHON_SITE_DIR}"
	)

	install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/onrisc.py ${CMAKE_CURRENT_BINARY_DIR}/_onrisc.so
	   DESTINATION ${PYTHON_SITE_DIR}
	   COMPONENT library
	)
endif (PYTHON_WRAP)
