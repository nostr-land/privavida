list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/relative.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_parse.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_write.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_write.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hex.cpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
