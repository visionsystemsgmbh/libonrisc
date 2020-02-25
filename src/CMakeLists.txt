# library
add_library(onrisc ${SRCS} ${PROJECT_SOURCE_DIR}/include/onrisc.h)
set_target_properties(onrisc PROPERTIES
	PUBLIC_HEADER "${PROJECT_BINARY_DIR}/onrisc.h"
	VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}
	SOVERSION 1)
target_link_libraries(onrisc ${LIBS})

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
	configure_file(${PROJECT_SOURCE_DIR}/swig/package.json.in
		       ${SWIG_NODEJS}/package.json @ONLY)

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
		DESTINATION ${SWIG_NODEJS})

	add_custom_command(
		TARGET onrisc
		POST_BUILD
		COMMAND swig -c++ -javascript -node -outcurrentdir
		-I${CMAKE_SOURCE_DIR}/src ${SWIG_ONRISC_I}
		WORKING_DIRECTORY ${SWIG_NODEJS}
		COMMENT "Creating Node.js bindings: swig")
endif (NODEJS_WRAP)
