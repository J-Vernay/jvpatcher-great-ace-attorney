// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "TGAAC_files.hpp"

#include <zlib.h>

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

ARC_Archive ARC_Load(Stream& arc)
{
    // 1. Read header

    ARC_FileHeader header;
    arc.Seek(0, SEEK_SET);
    arc.Read(std::span{&header, 1});

    if (memcmp(header.magic, "ARC\0", 4) != 0)
        throw RuntimeError("{}: Not starting with 'ARC\0'", arc.Name());

    if (header.version != 7 && header.version != 8)
        throw RuntimeError("{}: Bad ARC version {}", arc.Name(), header.version);

    // 2. Determine whether we have ARC_FileEntry or ARC_FileEntryExtendedName

    bool hasExtendedNames = [&] {
        ARC_FileEntry firstEntry;
        arc.Read(std::span{&firstEntry, 1});
        arc.Seek(-sizeof(firstEntry), SEEK_CUR);
        return firstEntry.extensionHash == 0 || firstEntry.decompSize == 0 ||
               firstEntry.offset == 0;
    }();

    // 3. Parse array of entries

    ARC_Archive result;
    result.version = header.version;
    result.entries.reserve(header.entryCount);

    auto funcReadEntries = [&]<typename TFileEntry>() {
        std::vector<TFileEntry> entries(header.entryCount);
        arc.Read(std::span{entries});
        for (TFileEntry& e : entries)
        {
            ARC_Entry& entry = result.entries.emplace_back();
            entry.filename = e.fileName;
            entry.ext = ARC_ExtensionHash{e.extensionHash};
            entry.decompSize = e.decompSize & 0x00FFFFFF;
            entry.content.resize(e.compSize);
            arc.Seek(e.offset, SEEK_SET);
            arc.Read(std::span{entry.content});
        }
    };

    if (hasExtendedNames)
        funcReadEntries.operator()<ARC_FileEntryExtendedName>();
    else
        funcReadEntries.operator()<ARC_FileEntry>();

    return result;
}

VecByte ARC_DecompressEntry(ARC_Entry const& entry)
{
    if (entry.content.size() == entry.decompSize)
        return entry.content;

    uint8_t magic = (uint8_t)entry.content[0];
    if ((magic & 0x0F) != 8 || (magic & 0xF0) > 0x70)
        throw RuntimeError("Unexpected decompression first byte: {}", magic);

    VecByte output(entry.decompSize);
    z_stream strm = {};
    strm.next_in = (unsigned char*)entry.content.data();
    strm.avail_in = entry.content.size();
    strm.next_out = (unsigned char*)output.data();
    strm.avail_out = output.size();

    int res = inflateInit(&strm);
    if (res != Z_OK)
        throw RuntimeError("Error with ZLIB inflateInit: {}", res);

    res = inflate(&strm, Z_NO_FLUSH);
    if (res != Z_STREAM_END)
        throw RuntimeError("Error with ZLIB inflate: {}", res);

    if (strm.avail_in != 0 || strm.avail_out != 0)
        throw RuntimeError(
            "Error with decompression, bytes remaining: in={} out={}", strm.avail_in,
            strm.avail_out);

    res = inflateEnd(&strm);
    if (res != Z_OK)
        throw RuntimeError("Error with ZLIB inflatEnd: {}", res);

    return output;
}