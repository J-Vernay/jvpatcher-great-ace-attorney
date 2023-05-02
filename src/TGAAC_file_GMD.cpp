// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "TGAAC_file_GMD.hpp"
#include "Utility.hpp"
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// GMD parser based on
// https://github.com/IcySon55/Kuriimu/blob/master/src/text/text_gmd/GMDv2.cs

struct GMD_FileHeader
{
    char magic[4];
    uint32_t version;
    uint32_t language;
    uint32_t padding[2];
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

void GMD_Registry::Load(stream_ptr& gmd)
{
    *this = {};

    // 1. Parse header
    GMD_FileHeader header;
    int64_t fileSize = gmd.SeekInput(0, std::ios::end);
    gmd.SeekInput(0, std::ios::beg);
    gmd.Read(std::span{&header, 1});

    if (memcmp(header.magic, "GMD\0", 0) != 0)
        gmd.Error("not starting with 'GMD\0'");

    if (header.version != 0x010302)
        gmd.Error("bad GMD version {:#x}", header.version);

    if (header.labelCount != header.sectionCount)
        gmd.Error("unsupported labelCount != sectionCount ({} != {})", header.labelCount,
                  header.sectionCount);

    // 2. Parse name

    name = gmd.ReadCStr();
    if (name.size() != header.nameSize)
        gmd.Error("ameSize mismatch (in header {}, found {})", header.nameSize,
                  name.size());

    // 3. Parse label entries

    std::vector<GMD_FileLabelEntry> labelEntries(header.labelCount);
    gmd.Read(std::span{labelEntries});

    // 4. Compute sizes

    int64_t prefixSize = gmd.SeekInput(0, std::ios::cur);
    int64_t bucketsSize = header.labelCount > 0 ? sizeof(GMD_FileBuckets) : 0;
    int64_t textSize = header.labelSize + header.sectionSize;
    int64_t expectedFileSize = prefixSize + bucketsSize + textSize;
    if (expectedFileSize != fileSize)
        gmd.Error("bad file size {} (expected {})", fileSize, expectedFileSize);

    // 5. Skip bucket (only necessary for fast random access)

    GMD_FileBuckets gmd_buckets{};
    if (bucketsSize)
        gmd.Read(std::span{&gmd_buckets, 1});

    // 6. Read all labels, store their labelOffset

    int64_t labelBegin = gmd.SeekInput(0, std::ios::cur);
    int64_t labelEnd = labelBegin + header.labelSize;

    std::unordered_map<int64_t, std::string> labels;
    while (true)
    {
        int64_t pos = gmd.SeekInput(0, std::ios::cur);
        if (pos >= labelEnd)
            break;
        int64_t offset = pos - labelBegin;
        std::string value = gmd.ReadCStr();
        labels[pos - labelBegin] = value;
    }
    int64_t pos = gmd.SeekInput(0, std::ios::cur);
    if (pos != labelEnd)
        gmd.Error("labelEnd does not match: {} != {}", pos, labelEnd);
    if (labels.size() != header.labelCount)
        gmd.Error("labelSize does not match: {} != {}", labels.size(), header.labelCount);

    // 7. Read all sections

    std::vector<std::string> sections;
    sections.reserve(header.sectionCount);
    for (uint32_t i = 0; i < header.sectionCount; ++i)
    {
        sections.emplace_back(gmd.ReadCStr());
    }
    if (gmd.SeekInput(0, std::ios::cur) != fileSize)
        gmd.Error("sectionSize does not match");

    // 8. Convert to output GMD_Registry

    version = header.version;
    language = header.language;
    name = (char*)name.data();
    entries.reserve(sections.size());
    for (size_t sectionID = 0; sectionID < sections.size(); ++sectionID)
    {
        GMD_Entry& entry = entries.emplace_back();
        std::string& section = sections[sectionID];
        entry.value = (char*)section.data();

        auto it = std::ranges::find_if(labelEntries, [&](GMD_FileLabelEntry const& e) {
            return e.sectionID == sectionID;
        });
        if (it == labelEntries.end())
            gmd.Error("could not find a label using sectionID {}", sectionID);

        if (!labels.contains(it->labelOffset))
            gmd.Error("Unknown label at offset {}", it->labelOffset);
        entry.key = std::move(labels.at(it->labelOffset));

        uint32_t hash0 = ~crc32(0, entry.key.data(), entry.key.size());
        uint32_t hash1 = ~crc32(~hash0, entry.key.data(), entry.key.size());
        uint32_t hash2 = ~crc32(~hash1, entry.key.data(), entry.key.size());
        if (hash1 != it->hash1)
            gmd.Error("hash1 mismatch: {} != {}", hash1, it->hash1);
        if (hash2 != it->hash2)
            gmd.Error("hash2 mismatch: {} != {}", hash2, it->hash2);
    }
}

void GMD_Registry::Save(stream_ptr& out) const
{
    // Header

    GMD_FileHeader gmd_header{};
    memcpy(gmd_header.magic, "GMD\0", 4);
    gmd_header.version = version;
    gmd_header.language = language;
    gmd_header.labelCount = entries.size();
    gmd_header.sectionCount = entries.size();
    gmd_header.labelSize = 0;   //< to be filled in the entries loop
    gmd_header.sectionSize = 0; //< to be filled in the entries loop
    gmd_header.nameSize = name.size();

    // Entries and BucketList

    std::vector<GMD_FileLabelEntry> gmd_labelEntries;
    gmd_labelEntries.reserve(entries.size());

    GMD_FileBuckets gmd_buckets{};

    // The bucket linked list in TGAAC files are from lower to upper indices.
    // Even if we could use the reverse order for simpler linked list code,
    // we want to have mostly byte-equal load/save files, for validation pruposes.
    //
    // So we need to store the last entry of each bucket linked list (the "tail"),
    // so we can update it to refer to the next entry.
    std::array<GMD_FileLabelEntry*, 256> bucketTails;
    bucketTails.fill(nullptr);

    int64_t offset = 0;
    for (uint64_t i = 0; i < entries.size(); ++i)
    {
        GMD_Entry const& entry = entries[i];
        GMD_FileLabelEntry& fileEntry = gmd_labelEntries.emplace_back();

        uint32_t hash0 = ~crc32(0, entry.key.data(), entry.key.size());
        uint32_t hash1 = ~crc32(~hash0, entry.key.data(), entry.key.size());
        uint32_t hash2 = ~crc32(~hash1, entry.key.data(), entry.key.size());

        uint8_t bucket = hash0 & 0xFF;
        GMD_FileLabelEntry* previous = std::exchange(bucketTails[bucket], &fileEntry);
        if (previous)
            previous->listLink = (i > 0 ? i : -1);
        else
            gmd_buckets.buckets[bucket] = (i > 0 ? i : -1);

        fileEntry.sectionID = i;
        fileEntry.hash1 = hash1;
        fileEntry.hash2 = hash2;
        fileEntry.zeroPadding = 0xCDCDCDCD;
        fileEntry.labelOffset = offset;
        fileEntry.listLink = 0;

        gmd_header.labelSize += entry.key.size() + 1;
        gmd_header.sectionSize += entry.value.size() + 1;
        offset += entry.key.size() + 1;
    }

    out.Write(std::span{&gmd_header, 1});
    out.Write(std::span{name.data(), name.size() + 1});
    out.Write(std::span{gmd_labelEntries});
    out.Write(std::span{&gmd_buckets, 1});

    for (GMD_Entry const& entry : entries)
        out.Write(std::span{entry.key.data(), entry.key.size() + 1});

    for (GMD_Entry const& entry : entries)
        out.Write(std::span{entry.value.data(), entry.value.size() + 1});

    out.Sync();
}

std::string GMD_EscapeEntryJV(std::string_view input)
{
    std::string result;
    result.reserve(input.size() * 1.5);

    // An abbreviation is <JV123>
    std::vector<std::pair<std::string, std::string_view>> abbrText;

    constexpr std::string_view SPECIAL = "\r\n<";
    constexpr std::string_view PAGE = "<PAGE>";

    while (input.size() > 0)
    {
        size_t notSpecialCount = input.find_first_of(SPECIAL);
        result += input.substr(0, notSpecialCount);
        if (notSpecialCount >= input.size())
            break;
        input.remove_prefix(notSpecialCount);

        if (input[0] == '\r')
        {
            input.remove_prefix(1);
        }
        else if (input[0] == '\n')
        {
            result += "<LINE/>\n";
            input.remove_prefix(1);
        }
        else
        {
            size_t specialCount = 0;
            for (std::string_view next = input;
                 next.starts_with('<') && !next.starts_with(PAGE);)
            {
                size_t closingPos = next.find('>');
                if (closingPos == input.npos)
                    throw runtime_error("Unclosed angle brackets");
                specialCount += closingPos + 1;
                next.remove_prefix(closingPos + 1);
            }
            if (specialCount > 0)
            {
                std::string_view special = input.substr(0, specialCount);
                input.remove_prefix(specialCount);
                std::string abbr = fmt::format("<JV{}/>", abbrText.size());
                result += abbr;
                abbrText.emplace_back(std::move(abbr), special);
            }
            if (input.starts_with(PAGE))
            {
                result += "<PAGE/>\n\n";
                input.remove_prefix(PAGE.size());
                while (input.starts_with('\r') || input.starts_with('\n'))
                    input.remove_prefix(1);
            }
        }
    }

    /// Used to know when a file has been modified.
    int64_t originalHash = std::hash<std::string>{}(result);

    using namespace rapidjson;
    StringBuffer buffer;
    PrettyWriter<StringBuffer> json(buffer);
    json.StartObject();
    json.Key("__originalHash__");
    json.Int64(originalHash);
    for (auto& [abbr, text] : abbrText)
    {
        json.Key(abbr.c_str());
        json.String(text.data(), text.size());
    }
    json.EndObject();

    result += "\n<!--\n==========JV==========\n";
    result += buffer.GetString();
    result += "\n-->";

    return result;
}