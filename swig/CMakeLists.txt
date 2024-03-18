if (POLICY CMP0078)
  cmake_policy(SET CMP0078 NEW)
endif()

if (POLICY CMP0086)
  cmake_policy(SET CMP0086 NEW)
endif()

# SWIG: Python
if (PYTHON_WRAP)
  find_package(SWIG REQUIRED)
  include(${SWIG_USE_FILE})

  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)
  include_directories(${PYTHON_INCLUDE_PATH})

  include_directories(${CMAKE_CURRENT_SOURCE_DIR})

  set(CMAKE_SWIG_FLAGS "")

  swig_add_library(
    pyonrisc
    LANGUAGE python
    SOURCES ${SWIG_ONRISC_I} ${SRCS}
  )
  swig_link_libraries(
    pyonrisc
    ${PYTHON_LIBRARIES}
    ${PC_LIBSOC_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
  )

endif (PYTHON_WRAP)

# SWIG: Lua
if (LUA_WRAP)
  find_package(SWIG REQUIRED)
  include(${SWIG_USE_FILE})

  find_package(Lua51 REQUIRED)
  include_directories(${LUA_INCLUDE_DIR})

  include_directories(${CMAKE_CURRENT_SOURCE_DIR})

  set(CMAKE_SWIG_FLAGS "")

  swig_add_library(
    luaonrisc
    LANGUAGE lua
    SOURCES ${SWIG_ONRISC_I} ${SRCS}
  )
  swig_link_libraries(
    luaonrisc
    ${LUA_LIBRARY}
    ${PC_LIBSOC_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
  )
endif (LUA_WRAP)

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

  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/onrisc.py
      ${CMAKE_CURRENT_BINARY_DIR}/_pyonrisc.so
    DESTINATION ${PYTHON_SITE_DIR}
    COMPONENT library
  )
endif (PYTHON_WRAP)
