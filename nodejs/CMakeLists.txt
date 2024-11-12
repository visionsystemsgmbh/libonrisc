set(NODEJS_WORKDIR "${CMAKE_BINARY_DIR}/nodejs")
file(MAKE_DIRECTORY ${NODEJS_WORKDIR})
configure_file(
  ${PROJECT_SOURCE_DIR}/nodejs/package.json.in
  ${NODEJS_WORKDIR}/package.json @ONLY
)
file(
  COPY
    "${CMAKE_SOURCE_DIR}/src/devices.c"
    "${CMAKE_SOURCE_DIR}/src/dips.c"
    "${CMAKE_SOURCE_DIR}/src/gpios.c"
    "${CMAKE_SOURCE_DIR}/src/leds.c"
    "${CMAKE_SOURCE_DIR}/src/onrisc.c"
    "${CMAKE_SOURCE_DIR}/src/system.c"
    "${CMAKE_SOURCE_DIR}/src/uarts.c"
    "${CMAKE_SOURCE_DIR}/include/vssys.h"
    "${CMAKE_SOURCE_DIR}/nodejs/index.js"
    "${CMAKE_SOURCE_DIR}/nodejs/binding.gyp"
    "${CMAKE_SOURCE_DIR}/nodejs/onrisc_wrap.cxx"
    "${CMAKE_SOURCE_DIR}/nodejs/.npmignore"
  DESTINATION ${NODEJS_WORKDIR}
)
