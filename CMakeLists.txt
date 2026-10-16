cmake_minimum_required(VERSION 3.25)
project(cerf LANGUAGES CXX)

add_subdirectory(libservicelocator)
set(CERFENGINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(libCerfEngine)
add_subdirectory(libCerfCore)
add_subdirectory(libCerfFrontend)

option(CERF_BUILD_QT "Build the cerf-qt frontend" OFF)
if(CERF_BUILD_QT)
    add_subdirectory(cerf-qt)
endif()
