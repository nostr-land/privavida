list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenizedContent.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenizedContent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VirtualizedList.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VirtualizedList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatMessage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ChatMessage.cpp
)
set(SOURCES ${SOURCES} PARENT_SCOPE)
