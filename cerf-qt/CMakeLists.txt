cmake_minimum_required(VERSION 3.25)
project(cerfqt LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

file(GLOB cerfqt_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(cerf-qt ${cerfqt_sources})
target_compile_features(cerf-qt PRIVATE cxx_std_20)
target_link_libraries(cerf-qt PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,cerffrontend::cerffrontend>"
    Qt6::Widgets
    Qt6::Svg)

set(cerfqt_assets "${CMAKE_CURRENT_SOURCE_DIR}/../cerf-win32/assets")
file(GLOB cerfqt_icons CONFIGURE_DEPENDS "${cerfqt_assets}/icons_sources/*.svg")
qt_add_resources(cerf-qt "cerfqt_assets"
    PREFIX "/"
    BASE "${cerfqt_assets}"
    FILES
        ${cerfqt_icons}
        "${cerfqt_assets}/cerf_1024.png"
        "${cerfqt_assets}/fonts/IBMPlexSans-Regular.ttf"
        "${cerfqt_assets}/fonts/IBMPlexSans-Bold.ttf"
        "${cerfqt_assets}/fonts/IBMPlexMono-Bold.ttf")

if(MSVC)
    target_compile_options(cerf-qt PRIVATE /W4 /WX /permissive- /utf-8)
else()
    target_compile_options(cerf-qt PRIVATE -Wall -Wextra -Werror)
    set_target_properties(cerf-qt PROPERTIES ENABLE_EXPORTS ON)
endif()

add_custom_target(cerf-qt-bundled ALL
    COMMAND "${CMAKE_COMMAND}"
            "-DCERFQT_BUNDLED_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../bundled"
            "-DCERFQT_DB_JSON=${CMAKE_CURRENT_SOURCE_DIR}/../libCerfCore/db.json"
            "-DCERFQT_OUTPUT_DIR=$<TARGET_FILE_DIR:cerf-qt>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/copy_bundled.cmake"
    VERBATIM)
add_dependencies(cerf-qt-bundled cerf-qt)
