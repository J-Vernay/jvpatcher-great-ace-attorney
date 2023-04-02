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

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using String = std::string;
using VecByte = std::vector<std::byte>;
using Path = std::filesystem::path;

enum class ExtensionHash : uint32_t
{
};

struct ARC_Entry
{
    String filename;   ///< Entry name, without extension
    ExtensionHash ext; ///< Number representing file type
    VecByte content;   ///< Byte content of the file
};

struct ARC_Archive
{
    uint16_t version;
    std::vector<ARC_Entry> entries;
};

/// Throws std::runtime_error on failure.
ARC_Archive ARC_LoadFromFile(Path const& arcFilePath);

#endif