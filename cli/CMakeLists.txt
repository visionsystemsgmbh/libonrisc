# onrisctool
add_executable(onrisctool ${PROJECT_SOURCE_DIR}/cli/onrisctool.c)
target_link_libraries(onrisctool onrisc)

# onriscdipd
add_executable(onriscdipd ${PROJECT_SOURCE_DIR}/cli/onriscdipd.c)
target_link_libraries(onriscdipd onrisc)

# onriscgpiod
add_executable(onriscgpiod ${PROJECT_SOURCE_DIR}/cli/onriscgpiod.c)
target_link_libraries(onriscgpiod onrisc)

install(TARGETS onrisctool RUNTIME DESTINATION bin)
install(TARGETS onriscdipd RUNTIME DESTINATION bin)
install(TARGETS onriscgpiod RUNTIME DESTINATION bin)
