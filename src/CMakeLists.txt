list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/app.h
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scroll_gesture.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scroll_gesture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tap_gesture.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tap_gesture.cpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
