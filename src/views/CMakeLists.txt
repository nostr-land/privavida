list(APPEND SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SubView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Root.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Root.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Conversations.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Conversations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScrollView.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ScrollView.cpp
)
add_subdirectory(ChatView)
add_subdirectory(MessageInspect)
add_subdirectory(TextInput)
add_subdirectory(TextRender)
set(SOURCES ${SOURCES} PARENT_SCOPE)
