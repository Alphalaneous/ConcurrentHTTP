cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)

project(ConcurrentHTTPMod)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp libraries/*.cpp libraries/*.c)


add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} )
# this is so stupid
set(CMAKE_SIZEOF_VOID_P 4)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()
 
target_include_directories(${PROJECT_NAME} PRIVATE
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h/
  libraries/yasio/
  libraries/openssl/include
  libraries/openssl/
  libraries/curl/include
  libraries/curl/
  libraries/uthash/
  libraries/concurrentqueue/
  libraries/llhttp/
  libraries/llhttp/include
  libraries/cocos-headers/extensions

)


add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)


target_link_libraries(
	${PROJECT_NAME} 
	minhook 
	cocos2d 
	${CMAKE_CURRENT_SOURCE_DIR}/libraries/curl/prebuilt/win/x86/libcurl_imp.lib 
	${CMAKE_CURRENT_SOURCE_DIR}/libraries/openssl/prebuilt/win/x86/libssl.lib 
	${CMAKE_CURRENT_SOURCE_DIR}/libraries/openssl/prebuilt/win/x86/libcrypto.lib
)






