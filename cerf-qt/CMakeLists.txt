cmake_minimum_required(VERSION 3.25)
project(cerfqt LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB cerfqt_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(cerf-qt ${cerfqt_sources})
target_compile_features(cerf-qt PRIVATE cxx_std_20)
target_link_libraries(cerf-qt PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,cerffrontend::cerffrontend>"
    Qt6::Widgets)

if(MSVC)
    target_compile_options(cerf-qt PRIVATE /W4 /WX /permissive- /utf-8)
else()
    target_compile_options(cerf-qt PRIVATE -Wall -Wextra -Werror)
endif()
