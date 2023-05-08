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
    bool isCompressed;     ///< Is the "content" field compressed with deflate algorithm.
    uint8_t unknownFlags;  ///< Unknown, vary among ARC entries, so probably some flags.

    static std::string Decompress(std::string_view input, uint32_t decompSize);
    static std::string Compress(std::string_view input);

    bool operator==(ARC_Entry const&) const noexcept = default;
};

struct ARC_Archive
{
    uint16_t version;
    std::vector<ARC_Entry> entries;
    bool hasExtendedNames;

    void Load(stream_ptr& in);
    void Save(stream_ptr& out) const;
    void ReadFiles(fs::path const& inFolder);
    void WriteFiles(fs::path const& outFolder) const;

    bool operator==(ARC_Archive const&) const noexcept = default;
};

#endif