add_definitions("-Wreturn-type")

# library
add_library(onrisc ${SRCS})
set_target_properties(onrisc PROPERTIES
  PUBLIC_HEADER "${PROJECT_BINARY_DIR}/onrisc.h"
  VERSION ${CMAKE_PROJECT_VERSION}
  SOVERSION 1)
target_link_libraries(onrisc ${LIBS})

install(TARGETS onrisc
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include COMPONENT dev
)
