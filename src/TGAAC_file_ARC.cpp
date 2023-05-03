// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "TGAAC_file_ARC.hpp"

// Based on Kuriimu2 :
// https://github.com/FanTranslatorsInternational/Kuriimu2/tree/dev/plugins/Capcom/plugin_mt_framework/Archives

struct ARC_FileHeader
{
    char magic[4];
    uint16_t version;
    uint16_t entryCount;
};

struct ARC_FileEntry
{
    char fileName[64];
    uint32_t extensionHash;
    int32_t compSize;
    int32_t decompSize;
    int32_t offset;
};

struct ARC_FileEntryExtendedName
{
    char fileName[128];
    uint32_t extensionHash;
    int32_t compSize;
    int32_t decompSize;
    int32_t offset;
};

void ARC_Archive::Load(stream_ptr& arc)
{
    // 1. Read header

    ARC_FileHeader header;
    arc.Read(std::span{&header, 1});

    if (memcmp(header.magic, "ARC\0", 4) != 0)
        arc.Error("not starting with 'ARC\0'");

    if (header.version != 7 && header.version != 8)
        arc.Error("bad ARC version {}", header.version);

    // 2. Determine whether we have ARC_FileEntry or ARC_FileEntryExtendedName

    hasExtendedNames = [&] {
        ARC_FileEntry firstEntry;
        arc.Read(std::span{&firstEntry, 1});
        arc.SeekInput(-sizeof(firstEntry), std::ios::cur);
        return firstEntry.extensionHash == 0 || firstEntry.decompSize == 0 ||
               firstEntry.offset == 0;
    }();

    // 3. Parse array of entries

    version = header.version;
    entries.reserve(header.entryCount);

    auto funcReadEntries = [&]<typename TFileEntry>() {
        std::vector<TFileEntry> fileEntries(header.entryCount);
        arc.Read(std::span{fileEntries});
        for (TFileEntry& e : fileEntries)
        {
            ARC_Entry& entry = entries.emplace_back();
            entry.filename = e.fileName;
            entry.ext = ARC_ExtensionHash{e.extensionHash};
            entry.decompSize = e.decompSize & 0x00FFFFFF;
            entry.unknownFlags = (e.decompSize >> 24);
            entry.content.resize(e.compSize);
            arc.SeekInput(e.offset, std::ios::beg);
            arc.Read(std::span{entry.content});
        }
    };

    if (hasExtendedNames)
        funcReadEntries.operator()<ARC_FileEntryExtendedName>();
    else
        funcReadEntries.operator()<ARC_FileEntry>();
}

void ARC_Archive::Save(stream_ptr& out) const
{
    ARC_FileHeader arc_header;
    memcpy(arc_header.magic, "ARC\0", 4);
    arc_header.version = version;
    arc_header.entryCount = entries.size();
    out.Write(std::span{&arc_header, 1});

    auto funcReadEntries = [&]<typename TFileEntry>() {
        std::vector<TFileEntry> fileEntries;
        fileEntries.reserve(entries.size());

        int64_t contentBase = sizeof(ARC_FileHeader);
        contentBase += entries.size() * sizeof(TFileEntry);
        contentBase += (-contentBase) & 0x7FFF; // alignas(0x8000)
        int64_t contentOffset = contentBase;

        for (ARC_Entry const& entry : entries)
        {
            TFileEntry& e = fileEntries.emplace_back();
            if (entry.filename.size() >= sizeof(e.fileName))
                out.Error("Filename size {} too big", entry.filename.size());
            memcpy(e.fileName, entry.filename.c_str(), entry.filename.size());
            e.extensionHash = (uint32_t)entry.ext;
            e.compSize = entry.content.size();
            e.decompSize = entry.decompSize | ((uint32_t)entry.unknownFlags << 24);
            e.offset = contentOffset;

            contentOffset += entry.content.size();
        }
        out.Write(std::span{fileEntries});
        int64_t pos = out.SeekOutput(0, std::ios::cur);
        std::vector<char> padding(contentBase - pos, 0);
        out.Write(std::span{padding});
    };

    if (hasExtendedNames)
        funcReadEntries.operator()<ARC_FileEntryExtendedName>();
    else
        funcReadEntries.operator()<ARC_FileEntry>();

    for (ARC_Entry const& entry : entries)
        out.Write(std::span{entry.content});
}

#include <zlib.h>

std::string ARC_Entry::Decompress(std::string_view input, uint32_t decompSize)
{
    if (input.size() == decompSize)
        return std::string{input.data(), input.size()};

    uint8_t magic = (uint8_t)input[0];
    if ((magic & 0x0F) != 8 || (magic & 0xF0) > 0x70)
        throw runtime_error("Unexpected decompression first byte: {}", magic);

    std::string output;
    output.resize(decompSize);
    z_stream strm = {};
    strm.next_in = (Bytef*)input.data();
    strm.avail_in = input.size();
    strm.next_out = (Bytef*)output.data();
    strm.avail_out = output.size();

    int res = inflateInit(&strm);
    if (res != Z_OK)
        throw runtime_error("Error with ZLIB inflateInit: {}", res);

    res = inflate(&strm, Z_NO_FLUSH);
    if (res != Z_STREAM_END)
        throw runtime_error("Error with ZLIB inflate: {}", res);

    if (strm.avail_in != 0 || strm.avail_out != 0)
        throw runtime_error("Error with decompression, bytes remaining: in={} out={}",
                            strm.avail_in, strm.avail_out);

    res = inflateEnd(&strm);
    if (res != Z_OK)
        throw runtime_error("Error with ZLIB inflatEnd: {}", res);

    return output;
}