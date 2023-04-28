list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/animation.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/animation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text_rendering.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/text_rendering.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stackbuffer.hpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
