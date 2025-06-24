# SWIG: Python
if(PYTHON_WRAP)
  find_package(SWIG REQUIRED)
  include(${SWIG_USE_FILE})

  find_package(Python3 COMPONENTS Interpreter Development)
  include_directories(${Python3_INCLUDE_DIRS})

  include_directories(${CMAKE_CURRENT_SOURCE_DIR})

  set(CMAKE_SWIG_FLAGS "")

  swig_add_library(
    pyonrisc
    LANGUAGE python
    SOURCES ${SWIG_ONRISC_I} ${SRCS}
  )
  swig_link_libraries(
    pyonrisc
    ${Python3_LIBRARIES}
    ${PC_LIBSOC_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
  )

endif()

# SWIG: Lua
if(LUA_WRAP)
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
endif()

if(PYTHON_WRAP)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c
    "import site, sys; sys.stdout.write(site.PREFIXES[-1])"
    OUTPUT_VARIABLE PYTHON_PREFIX
  )
  file(TO_CMAKE_PATH "${PYTHON_PREFIX}" PYTHON_PREFIX)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c
    "import site, sys; sys.stdout.write(site.getsitepackages()[-1])"
    OUTPUT_VARIABLE PYTHON_SITE_DIR
  )
  file(TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR)
  string(REGEX REPLACE "^${PYTHON_PREFIX}/" ""
    PYTHON_SITE_DIR "${PYTHON_SITE_DIR}"
  )

  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/onrisc.py
      ${CMAKE_CURRENT_BINARY_DIR}/_pyonrisc.so
    DESTINATION ${PYTHON_SITE_DIR}
    COMPONENT library
  )
endif()
