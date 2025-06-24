cmake_minimum_required(VERSION 3.15)

project(libonrisc VERSION 1.8.0 LANGUAGES C)

include(CheckTypeSize)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(PYTHON_WRAP "Build Python bindings" OFF)
option(LUA_WRAP "Build Lua bindings" OFF)
option(NODEJS_WRAP "Build Node.js bindings" OFF)

# Create compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  # Auto-create a .gitignore in the out-of-source build directory.
  file(GENERATE OUTPUT .gitignore CONTENT "*")
endif()

set(CMAKE_EXTRA_INCLUDE_FILES linux/gpio.h)
check_type_size("struct gpiochip_info" STRUCT_GPIOCHIP_INFO)
set(CMAKE_EXTRA_INCLUDE_FILES)

if(NOT HAVE_STRUCT_GPIOCHIP_INFO)
  message(FATAL_ERROR "libonrisc requires a toolchain \
with GPIO character devices API (Linux header version > 4.5.x)")
endif()

# handle dependencies
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_LIBSOC REQUIRED libsoc)

include_directories(BEFORE
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PC_LIBSOC_INCLUDE_DIRS}
)
link_directories(${PC_LIBSOC_LIBDIR})

configure_file(
  ${PROJECT_SOURCE_DIR}/include/onrisc.h.in
  ${PROJECT_BINARY_DIR}/onrisc.h @ONLY
)

set(SRCS
  ${PROJECT_SOURCE_DIR}/src/devices.c
  ${PROJECT_SOURCE_DIR}/src/dips.c
  ${PROJECT_SOURCE_DIR}/src/gpios.c
  ${PROJECT_SOURCE_DIR}/src/leds.c
  ${PROJECT_SOURCE_DIR}/src/onrisc.c
  ${PROJECT_SOURCE_DIR}/src/system.c
  ${PROJECT_SOURCE_DIR}/src/uarts.c
)

set(SWIG_ONRISC_I ${PROJECT_SOURCE_DIR}/swig/onrisc.i)
set(LIBS
  ${PC_LIBSOC_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/cli)
add_subdirectory(${CMAKE_SOURCE_DIR}/swig)
if(NODEJS_WRAP)
  add_subdirectory(${CMAKE_SOURCE_DIR}/nodejs)
endif()

# PkgConfig
configure_file(
  ${CMAKE_SOURCE_DIR}/libonrisc.pc.in
  ${CMAKE_BINARY_DIR}/libonrisc.pc @ONLY
)
install(
  FILES ${CMAKE_BINARY_DIR}/libonrisc.pc
  DESTINATION lib${LIB_SUFFIX}/pkgconfig
)
