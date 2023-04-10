// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#ifndef JV_TGAAC_FILE_ARC_H
#define JV_TGAAC_FILE_ARC_H

#include "Utility.hpp"

enum class ARC_ExtensionHash : uint32_t
{
    GMD = 0x242BB29A
};

struct ARC_Entry
{
    std::string filename;  ///< Entry name, without extension
    ARC_ExtensionHash ext; ///< Number representing file type
    std::string content;   ///< Byte content of the file, may be compressed.
    uint32_t decompSize;   ///< The content size if decompressed.
};

struct ARC_Archive
{
    uint16_t version;
    std::vector<ARC_Entry> entries;
};

/// Throws std::runtime_error on failure.
ARC_Archive ARC_Load(stream_ptr& arc);

std::string ARC_DecompressEntry(ARC_Entry const& entry);

#endif