cmake_minimum_required (VERSION 3.13)
set(APP_NAME privavida-core)
project(${APP_NAME} CXX C)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)

set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON CACHE BOOL "Secp256k1 Enable module Schnorr signatures")
set(SECP256K1_DISABLE_SHARED ON CACHE BOOL "Disable shared library. Overrides BUILD_SHARED_LIBS.")
set(SECP256K1_ENABLE_MODULE_ECDH OFF CACHE BOOL "Enable ECDH module.")
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "Build benchmarks.")
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "Build tests.")
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "Build exhaustive tests.")

add_subdirectory(src)
add_subdirectory(lib/nanovg)
add_subdirectory(lib/sha256)
add_subdirectory(lib/websocketpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-abi-version=2 -fobjc-arc -std=gnu++17 -stdlib=libc++ -isysroot ${CMAKE_OSX_SYSROOT}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fobjc-abi-version=2 -fobjc-arc -isysroot ${CMAKE_OSX_SYSROOT}")

add_library(${APP_NAME} STATIC ${SOURCES})

target_include_directories(${APP_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/rapidjson/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/secp256k1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/websocketpp
)
