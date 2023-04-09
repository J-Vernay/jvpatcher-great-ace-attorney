cmake_minimum_required(VERSION 3.21)

project(TGAAC_jv_patcher)

find_package(ZLIB)
add_subdirectory(external/libfmt)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE external/rapidjson/include)
target_compile_definitions(rapidjson INTERFACE RAPIDJSON_NOMEMBERITERATORCLASS)

add_executable(TGAAC_jv_patcher
    src/main.cpp
    src/Utility.cpp
    src/TGAAC_file_ARC.cpp
    src/TGAAC_file_GMD.cpp
    src/TGAAC_actions.cpp
)
target_compile_features(TGAAC_jv_patcher PUBLIC cxx_std_20)
target_link_libraries(TGAAC_jv_patcher PUBLIC ZLIB::ZLIB)
target_link_libraries(TGAAC_jv_patcher PUBLIC fmt::fmt)
target_link_libraries(TGAAC_jv_patcher PUBLIC rapidjson)
