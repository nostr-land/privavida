list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/relative.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_parse.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/relay_message_parse.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/relay_message_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_stringify.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event_stringify.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/secp256k1.c
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
