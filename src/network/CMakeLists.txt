list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/network.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handle_event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/handle_event.cpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
