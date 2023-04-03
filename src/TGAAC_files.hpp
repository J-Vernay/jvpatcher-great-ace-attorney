#ifndef JV_TGAAC_FILE_ARC_H
#define JV_TGAAC_FILE_ARC_H

/// This file contains routines to open ARC files.
/// In this implementation, only the bare minimum is provided
/// for The Great Ace Attorney Chronicles (TGAAC) script files.

/// The content of TGAAC is stored in ARC files (archives),
/// and the script is stored in these ARC files, as GMD entries.

/// This work could not have been done without the preliminary
/// retro-engineering work for Kuriimu 1 & 2, especially:
/// ARC: https://github.com/FanTranslatorsInternational/Kuriimu2/tree/dev/plugins/Capcom/plugin_mt_framework/Archives
/// GMD: https://github.com/IcySon55/Kuriimu/tree/master/src/text/text_gmd

#include "Utility.hpp"

#pragma region ARC editing

enum class ARC_ExtensionHash : uint32_t
{
    GMD = 0x242BB29A
};

struct ARC_Entry
{
    String filename;       ///< Entry name, without extension
    ARC_ExtensionHash ext; ///< Number representing file type
    VecByte content;       ///< Byte content of the file, may be compressed.
    uint32_t decompSize;   ///< The content size if decompressed.
};

struct ARC_Archive
{
    uint16_t version;
    std::vector<ARC_Entry> entries;
};

/// Throws std::runtime_error on failure.
ARC_Archive ARC_LoadFromFile(Stream& arc);

VecByte ARC_DecompressEntry(ARC_Entry const& entry);

#pragma endregion

#pragma region GMD editing

struct GMD_Entry
{
    std::string key;
    std::string value;
    uint32_t hash1{}; ///< Some hash based on 'key'
    uint32_t hash2{}; ///< Some other hash based on 'key'
};

struct GMD_Registry
{
    uint32_t version;
    uint32_t language;
    std::string name;
    std::vector<GMD_Entry> entries;
};

GMD_Registry GMD_LoadFromFile(Stream& gmd);

#pragma endregion

#endif