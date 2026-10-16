cmake_minimum_required(VERSION 3.25)
project(cerffrontend VERSION 1.0.0 LANGUAGES CXX)

if(NOT TARGET cerfcore::cerfcore)
    message(FATAL_ERROR "libCerfFrontend builds inside a tree that provides cerfcore::cerfcore")
endif()

file(GLOB cerffrontend_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(cerffrontend STATIC ${cerffrontend_sources})
add_library(cerffrontend::cerffrontend ALIAS cerffrontend)
target_include_directories(cerffrontend PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(cerffrontend PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,cerfcore::cerfcore>")

if(MSVC)
    target_compile_options(cerffrontend PRIVATE /W4 /WX /permissive- /utf-8 /MP /Gy /Oi)
else()
    target_compile_options(cerffrontend PRIVATE -Wall -Wextra -Werror)
endif()
