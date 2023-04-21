list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/relative.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keys.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keys.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/nostr_entity.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nostr_entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nip04.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nip04.cpp
)
add_subdirectory(c)
set(SOURCES ${SOURCES} PARENT_SCOPE)
