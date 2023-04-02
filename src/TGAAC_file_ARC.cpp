#include "TGAAC_files.hpp"

#include <cstdio>

ARC_Archive ARC_LoadFromFile(Path const& arcFilePath)
{
    std::FILE* f = fopen(arcFilePath.c_str(), "rb");
    if (f == nullptr)
        throw std::runtime_error("Could not open ");

    throw std::runtime_error("Oups");
}