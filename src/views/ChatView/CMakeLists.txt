list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatViewEntry.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatViewEntry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VirtualizedList.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VirtualizedList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatMessage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatMessage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Composer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Composer.cpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
