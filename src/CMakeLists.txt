list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/app.cpp
)
add_subdirectory(views)
add_subdirectory(models)
add_subdirectory(utils)
add_subdirectory(network)
add_subdirectory(data_layer)
set(SOURCES ${SOURCES} PARENT_SCOPE)
