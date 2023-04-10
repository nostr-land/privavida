cmake_minimum_required (VERSION 2.8)
set(APP_NAME privavida-core)
project(${APP_NAME} CXX C)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
add_subdirectory(lib/nanovg)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-abi-version=2 -fobjc-arc -std=gnu++17 -stdlib=libc++ -isysroot ${CMAKE_OSX_SYSROOT}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fobjc-abi-version=2 -fobjc-arc -isysroot ${CMAKE_OSX_SYSROOT}")
#IF (${IS_SIMULATOR})
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mios-simulator-version-min=6.0")
#ENDIF()

add_library(${APP_NAME} STATIC ${SOURCES})

#set_xcode_property(${APP_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES "All")
target_include_directories(${APP_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
