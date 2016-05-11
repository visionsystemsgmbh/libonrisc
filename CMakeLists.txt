project (libonrisc C)

if(NOT VSOR_ARCH)
	SET (VSOR_ARCH "armhf")
endif(NOT VSOR_ARCH)

#CPack stuff
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OnRISC Hardware API Library")
SET(CPACK_PACKAGE_VENDOR "VScom")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "5")
SET(CPACK_PACKAGE_VERSION_PATCH "2")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_CONTACT "service@visionsystems.de")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${VSOR_ARCH})
INCLUDE(CPack)

cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBSOC REQUIRED libsoc)
pkg_check_modules(PC_LIBUDEV REQUIRED libudev)

include_directories(BEFORE ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/include ${PC_LIBSOC_INCLUDE_DIRS} ${PC_LIBUDEV_INCLUDE_DIRS})

configure_file(${PROJECT_SOURCE_DIR}/include/onrisc.h.in ${PROJECT_BINARY_DIR}/onrisc.h @ONLY)
set(SRCS src/onrisc.c src/leds.c src/gpios.c src/dips.c src/system.c src/uarts.c src/devices.c)

# swig 

if (PYTHON_WRAP)
	find_package(SWIG REQUIRED)
	include(${SWIG_USE_FILE})

	find_package(PythonInterp REQUIRED)
	find_package(PythonLibs REQUIRED)
	include_directories(${PYTHON_INCLUDE_PATH})

	include_directories(${CMAKE_CURRENT_SOURCE_DIR})

	set(CMAKE_SWIG_FLAGS "")

	swig_add_module(onrisc python onrisc.i ${SRCS})
	swig_link_libraries(onrisc ${PYTHON_LIBRARIES} soc pthread udev)
endif (PYTHON_WRAP)

if (LUA_WRAP)
	find_package(SWIG REQUIRED)
	include(${SWIG_USE_FILE})

	FIND_PACKAGE(Lua51 REQUIRED)
	INCLUDE_DIRECTORIES(${LUA_INCLUDE_DIR})

 	include_directories(${CMAKE_CURRENT_SOURCE_DIR})

	set(CMAKE_SWIG_FLAGS "")

	swig_add_module(luaonrisc lua onrisc.i ${SRCS})
	swig_link_libraries(luaonrisc soc pthread udev ${LUA_LIBRARY})
endif (LUA_WRAP)

# library
add_library(onrisc SHARED ${SRCS} ${PROJECT_SOURCE_DIR}/include/onrisc.h)
set_target_properties(onrisc PROPERTIES
	PUBLIC_HEADER "${PROJECT_BINARY_DIR}/onrisc.h"
	VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
	SOVERSION 1)
target_link_libraries(onrisc ${PC_LIBSOC_LIBRARIES} ${PC_LIBUDEV_LIBRARIES} pthread)

# cli
add_executable(onrisctool ${PROJECT_SOURCE_DIR}/cli/onrisctool.c)
target_link_libraries(onrisctool onrisc)

# cli
add_executable(onriscdipd ${PROJECT_SOURCE_DIR}/cli/onriscdipd.c)
target_link_libraries(onriscdipd onrisc)

install(TARGETS onrisc 
	LIBRARY DESTINATION lib COMPONENT shlib
	PUBLIC_HEADER DESTINATION include COMPONENT dev
	)
install(TARGETS onrisctool RUNTIME DESTINATION bin)
install(TARGETS onriscdipd RUNTIME DESTINATION bin)

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
