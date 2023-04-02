#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <zlib.h>

namespace fs = std::filesystem;

fs::path const TGAAC_DIR = "/home/jvernay/.local/share/Steam/steamapps/common/TGAAC";
fs::path const ARCHIVE_DIR = TGAAC_DIR / "nativeDX11x64/archive";

struct MtHeader
{
    char magic[4];
    uint16_t version;
    uint16_t entryCount;
};

struct MtEntry
{
    char fileName[64];
    uint32_t extensionHash;
    int32_t compSize;
    int32_t decompSize;
    int32_t offset;
};

struct MtEntryExtendedName
{
    char fileName[128];
    uint32_t extensionHash;
    int32_t compSize;
    int32_t decompSize;
    int32_t offset;
};

struct MtEntrySwitch
{
    char fileName[64];
    uint32_t extensionHash;
    int32_t compSize;
    int32_t decompSize;
    int32_t unk1;
    int32_t offset;
};

int main()
{
    //for (fs::directory_entry e : fs::directory_iterator(ARCHIVE_DIR))
    //    std::cout << e.path() << '\n';

    fs::path ARC = ARCHIVE_DIR / "title_select_eng.arc";

    std::cout << fs::file_size(ARC) << '\n';

    FILE* f = fopen(ARC.c_str(), "rb");

    char _discard[128];

    MtHeader header;
    fread(&header, sizeof(header), 1, f);

    if (header.version != 7 && header.version != 8)
        fread(_discard, sizeof(int32_t), 1, f);

    MtEntry firstEntry;
    fread(&firstEntry, sizeof(MtEntry), 1, f);
    bool hasExtendedName = firstEntry.extensionHash == 0 || firstEntry.decompSize == 0 ||
                           firstEntry.offset == 0;

    fseek(f, -sizeof(MtEntry), SEEK_CUR);

    struct Entry
    {
        std::string filename;
        size_t offset;
        size_t compSize;
        size_t decompSize;
        std::vector<char> content;
    };
    std::vector<Entry> entries;
    entries.reserve(header.entryCount);

    if (hasExtendedName)
    {
        std::vector<MtEntryExtendedName> vec;
        vec.resize(header.entryCount);
        fread(vec.data(), sizeof(MtEntryExtendedName), vec.size(), f);
        for (auto& e : vec)
        {
            if (e.extensionHash == 0x242BB29A)
            {
                Entry& entry = entries.emplace_back();
                entry.filename = e.fileName;
                entry.filename += ".gmd";
                entry.offset = e.offset;
                entry.compSize = e.compSize;
                entry.decompSize = e.decompSize & 0x00FFFFFF;
            }
        }
    }
    else
    {
        std::vector<MtEntry> vec;
        vec.resize(header.entryCount);
        fread(vec.data(), sizeof(MtEntry), vec.size(), f);
        for (auto& e : vec)
        {
            if (e.extensionHash == 0x242BB29A)
            {
                Entry& entry = entries.emplace_back();
                entry.filename = e.fileName;
                entry.filename += ".gmd";
                entry.offset = e.offset;
                entry.compSize = e.compSize;
                entry.decompSize = e.decompSize & 0x00FFFFFF;
            }
        }
    }

    for (auto& entry : entries)
    {
        fseek(f, entry.offset, SEEK_SET);

        entry.content.resize(entry.compSize);
        fread(entry.content.data(), 1, entry.compSize, f);
        if (entry.compSize != entry.decompSize)
        {
            printf("Decompression required for %s\n", entry.filename.c_str());
            char magic = entry.content[0];
            if ((magic & 0x0F) != 8 || (magic & 0xF0) > 0x70)
                printf("ERROR\n");
            std::vector<char> output;
            output.resize(entry.decompSize);
            z_stream strm = {};
            strm.next_in = (Bytef*)entry.content.data();
            strm.avail_in = entry.content.size();
            strm.next_out = (Bytef*)output.data();
            strm.avail_out = output.size();
            inflateInit(&strm);
            inflate(&strm, Z_NO_FLUSH);
            inflateEnd(&strm);
            entry.content = std::move(output);
        }
    }

    fclose(f);
}
