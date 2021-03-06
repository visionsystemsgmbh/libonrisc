project (libonrisc C)

if(NOT VSOR_ARCH)
	SET (VSOR_ARCH "armhf")
endif(NOT VSOR_ARCH)

#CPack stuff
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OnRISC Hardware API Library")
SET(CPACK_PACKAGE_VENDOR "VScom")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "6")
SET(CPACK_PACKAGE_VERSION_PATCH "2")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libsoc | libsoc2")
SET(CPACK_PACKAGE_CONTACT "service@visionsystems.de")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${VSOR_ARCH})
INCLUDE(CPack)

cmake_minimum_required(VERSION 3.10)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(PYTHON_WRAP "Build Python bindings" OFF)
option(LUA_WRAP "Build Lua bindings" OFF)
option(NODEJS_WRAP "Build Node.js bindings" OFF)

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
	
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/cli)
add_subdirectory(${CMAKE_SOURCE_DIR}/swig)
if (NODEJS_WRAP)
	add_subdirectory(${CMAKE_SOURCE_DIR}/nodejs)
endif (NODEJS_WRAP)

# PkgConfig
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(includedir  ${CMAKE_INSTALL_PREFIX}/include)
set(libdir      ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})

configure_file(${CMAKE_SOURCE_DIR}/libonrisc.pc.in ${CMAKE_BINARY_DIR}/libonrisc.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/libonrisc.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
