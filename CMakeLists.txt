project (libonrisc C)

if(NOT VSOR_ARCH)
	SET (VSOR_ARCH "armhf")
endif(NOT VSOR_ARCH)

#CPack stuff
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OnRISC Hardware API Library")
SET(CPACK_PACKAGE_VENDOR "VScom")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "4")
SET(CPACK_PACKAGE_VERSION_PATCH "1")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")
SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_CONTACT "service@visionsystems.de")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${VSOR_ARCH})
INCLUDE(CPack)

cmake_minimum_required(VERSION 2.6)

include_directories(BEFORE ${PROJECT_SOURCE_DIR}/include) 

set(SRCS src/onrisc.c src/leds.c src/gpios.c src/system.c src/uarts.c)

# swig 

FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(CMAKE_SWIG_FLAGS "")

SWIG_ADD_MODULE(onrisc python onrisc.i ${SRCS})
SWIG_LINK_LIBRARIES(onrisc ${PYTHON_LIBRARIES} soc pthread udev)

# library
add_library(onrisc SHARED ${SRCS} ${PROJECT_SOURCE_DIR}/include/onrisc.h)
set_target_properties(onrisc PROPERTIES
	PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/onrisc.h")
target_link_libraries(onrisc soc pthread udev)

# cli
add_executable(onrisctool ${PROJECT_SOURCE_DIR}/cli/onrisctool.c)
target_link_libraries(onrisctool onrisc)

install(TARGETS onrisc 
	LIBRARY DESTINATION lib COMPONENT shlib
	PUBLIC_HEADER DESTINATION include COMPONENT dev
	)
install(TARGETS onrisctool RUNTIME DESTINATION bin)
