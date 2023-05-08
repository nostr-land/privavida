list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/accounts.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/accounts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/relays.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/relays.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/events.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/events.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/conversations.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/conversations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/profiles.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/profiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/contact_lists.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/contact_lists.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/images.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/images.cpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
