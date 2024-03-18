set(NODEJS_WORKDIR "${CMAKE_BINARY_DIR}/nodejs")
file(MAKE_DIRECTORY ${NODEJS_WORKDIR})
configure_file(
  ${PROJECT_SOURCE_DIR}/nodejs/package.json.in
  ${NODEJS_WORKDIR}/package.json @ONLY
)
file(
  COPY
    "${CMAKE_SOURCE_DIR}/nodejs/libonrisc.js"
    "${CMAKE_SOURCE_DIR}/nodejs/.npmignore"
  DESTINATION ${NODEJS_WORKDIR}
)
