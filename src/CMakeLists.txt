list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/app.h
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ui.cpp
)
add_subdirectory(views)
add_subdirectory(models)
add_subdirectory(utils)
add_subdirectory(network)
add_subdirectory(nostr)
set(SOURCES ${SOURCES} PARENT_SCOPE)
