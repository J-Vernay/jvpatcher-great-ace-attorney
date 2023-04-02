cmake_minimum_required(VERSION 3.21)

project(TGAAC_Patching)

find_package(ZLIB)

add_executable(TGAAC_patcher
    src/main.cpp
)
target_compile_features(TGAAC_patcher PUBLIC cxx_std_20)
target_link_libraries(TGAAC_patcher PUBLIC ZLIB::ZLIB)