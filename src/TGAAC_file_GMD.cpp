#include "TGAAC_files.hpp"
#include "Utility.hpp"

struct GMD_FileHeader
{
    char magic[4];
    uint32_t version;
    uint32_t language;
    uint32_t zero[2];
    uint32_t labelCount;
    uint32_t sectionCount;
    uint32_t labelSize;
    uint32_t sectionSize;
    uint32_t nameSize;
};

struct GMD_FileLabelEntry
{
    uint32_t sectionID;
    uint32_t hash1;
    uint32_t hash2;
    uint32_t zeroPadding;
    uint64_t labelOffset;
    uint64_t listLink;
};

struct GMD_FileBuckets
{
    uint64_t buckets[256];
};

GMD_Registry GMD_Load(Stream& gmd)
{
    // 1. Parse header

    GMD_FileHeader header;
    gmd.Seek(0, SEEK_END);
    int64_t fileSize = gmd.Tell();
    gmd.Seek(0, SEEK_SET);
    gmd.Read(std::span{&header, 1});

    if (memcmp(header.magic, "GMD\0", 0) != 0)
        throw RuntimeError("{}: Not starting with 'GMD\0'", gmd.Name());

    if (header.version != 0x010302)
        throw RuntimeError("{}: Bad GMD version {:#x}", gmd.Name(), header.version);

    // 2. Parse name

    VecByte name;
    gmd.ReadCStr(name);
    fmt::print("name = {}\n", (char*)name.data());

    // 3. Parse label entries

    std::vector<GMD_FileLabelEntry> labelEntries(header.labelCount);
    gmd.Read(std::span{labelEntries});

    // 4. Compute sizes

    int64_t prefixSize = gmd.Tell();
    int64_t bucketsSize = header.labelCount > 0 ? sizeof(GMD_FileBuckets) : 0;
    int64_t textSize = header.labelSize + header.sectionSize;
    int64_t expectedFileSize = prefixSize + bucketsSize + textSize;
    if (expectedFileSize != fileSize)
        throw RuntimeError(
            "{}: Bad file size {} (expected {})", gmd.Name(), fileSize, expectedFileSize);

    // 5. Skip bucket (only necessary for fast random access)
    if (bucketsSize)
        gmd.Seek(sizeof(GMD_FileBuckets), SEEK_CUR);

    // 6. Read all labels, store their labelOffset

    int64_t labelBegin = gmd.Tell();
    int64_t labelEnd = labelBegin + header.labelSize;

    std::unordered_map<int64_t, VecByte> labels;
    while (true)
    {
        int64_t pos = gmd.Tell();
        if (pos >= labelEnd)
            break;
        gmd.ReadCStr(labels[pos - labelBegin]);
    }
    if (gmd.Tell() != labelEnd)
        throw RuntimeError("{}: labelSize does not match", gmd.Name());

    // 7. Read all sections

    std::vector<VecByte> sections;
    sections.reserve(header.sectionCount);
    for (uint32_t i = 0; i < header.sectionCount; ++i)
    {
        VecByte& section = sections.emplace_back();
        gmd.ReadCStr(section);
    }
    if (gmd.Tell() != fileSize)
        throw RuntimeError("{}: sectionSize does not match", gmd.Name());

    // 8. Convert to output GMD_Registry

    GMD_Registry result;
    result.version = header.version;
    result.language = header.language;
    result.name = (char*)name.data();
    result.entries.reserve(sections.size());
    for (size_t sectionID = 0; sectionID < sections.size(); ++sectionID)
    {
        GMD_Entry& entry = result.entries.emplace_back();
        VecByte& section = sections[sectionID];
        entry.value = (char*)section.data();

        auto it = std::ranges::find_if(labelEntries, [&](GMD_FileLabelEntry const& e) {
            return e.sectionID == sectionID;
        });

        if (it != labelEntries.end())
        {
            VecByte const& label = labels.at(it->labelOffset);

            entry.hash1 = it->hash1;
            entry.hash2 = it->hash2;
            entry.key = (char*)label.data();
        }
    }

    return result;
}