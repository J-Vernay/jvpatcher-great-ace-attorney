cmake_minimum_required(VERSION 3.21)

project(TGAAC_Patching)

find_package(ZLIB)
add_subdirectory(external/libfmt)

add_executable(TGAAC_patcher
    src/main.cpp
    src/Utility.cpp
    src/TGAAC_file_ARC.cpp
    src/TGAAC_file_GMD.cpp
)
target_compile_features(TGAAC_patcher PUBLIC cxx_std_20)
target_link_libraries(TGAAC_patcher PUBLIC ZLIB::ZLIB)
target_link_libraries(TGAAC_patcher PUBLIC fmt::fmt)
#target_include_directories(TGAAC_patcher PUBLIC external/libfmt/include)