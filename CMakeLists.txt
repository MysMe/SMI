cmake_minimum_required(VERSION 3.19)
set(PROJECT_NAME SMI)
set(CMAKE_CXX_STANDARD 17)

if(NOT DEFINED OAUTHLIB)
  message(FATAL_ERROR "OAUTHLIB not defined.")
endif()

project(${PROJECT_NAME})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


add_executable(${PROJECT_NAME} src/Source.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
add_subdirectory(src)
add_subdirectory(include)

include(CMakeGroupSources.txt)

find_package(CURL CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

#oauth has an odd build method, build externally and link here
target_include_directories(${PROJECT_NAME} PRIVATE ${OAUTHLIB}/include)
if (CMAKE_BUILD_TYPE EQUAL "RELEASE")
	target_link_libraries(${PROJECT_NAME} PRIVATE ${OAUTHLIB}/build/Release/oauthcpp.lib)
else()
	target_link_libraries(${PROJECT_NAME} PRIVATE ${OAUTHLIB}/build/Debug/oauthcpp.lib)
endif() 