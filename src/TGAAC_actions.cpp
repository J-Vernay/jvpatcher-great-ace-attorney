// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "TGAAC_actions.hpp"
#include "TGAAC_file_ARC.hpp"
#include "TGAAC_file_GMD.hpp"
#include <algorithm>
#include <filesystem>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

static void _ensureEmptyFolder(fs::path const& folder)
{
    if (!fs::exists(folder))
        fs::create_directory(folder);
    if (!fs::is_directory(folder))
        throw runtime_error("Given patch folder exists and is not a directory!");

    for (auto&& p : fs::directory_iterator(folder))
        throw runtime_error("Given patch folder is not empty");
}

void TGAAC_ExtractGMD(stream_ptr& gmdStream, fs::path const& destFolder)
{
    // 1. Read and determine what is needed to be done.

    GMD_Registry gmd;
    gmd.Load(gmdStream);
    std::unordered_map<std::string, GMD_Entry const&> mapNameEntry;

    for (GMD_Entry const& entry : gmd.entries)
    {
        std::string name = ConvertToID(entry.key) + ".jv.xml";
        if (mapNameEntry.contains(name))
            throw runtime_error("Duplicated name '{}'", name);
        mapNameEntry.insert({name, entry});
    }

    // 2. Prepare workspace

    if (mapNameEntry.size() == 0)
        return;

    _ensureEmptyFolder(destFolder);

    // 3. Do actual work

    for (auto& [name, entry] : mapNameEntry)
    {
        fs::path dest = destFolder / name;
        FILE* f = fopen(dest.c_str(), "wb");
        if (f == nullptr)
            throw runtime_error("Could not open file {}", dest.native());

        //std::string value = GMD_EscapeEntryJV(entry.value);
        fwrite(entry.value.data(), 1, entry.value.size(), f);
        fclose(f);
    }

    // 4. Write metadata

    using namespace rapidjson;
    FILE* f = fopen((destFolder / "__META__.json").c_str(), "wb");
    if (f == nullptr)
        throw runtime_error("Could not open file META.json");

    char writeBuffer[65536];
    FileWriteStream os{f, writeBuffer, sizeof(writeBuffer)};
    PrettyWriter<FileWriteStream> json{os};

    json.StartObject();
    json.Key("version");
    json.Int64(gmd.version);
    json.Key("language");
    json.Int64(gmd.language);
    json.Key("name");
    json.String(gmd.name.c_str());
    json.Key("entries");
    json.StartObject();
    for (auto& [name, entry] : mapNameEntry)
    {
        json.Key(name.c_str());
        json.StartObject();
        json.Key("key");
        json.String(entry.key.c_str());
        json.Key("hash1");
        json.Int64(entry.hash1);
        json.Key("hash2");
        json.Int64(entry.hash2);
        json.EndObject();
    }
    json.EndObject();
    json.EndObject();
    fclose(f);
}

void TGAAC_ExtractARC(stream_ptr& arcStream, fs::path const& destFolder)
{
    // 1. Read and determine what is needed to be done.

    ARC_Archive arc;
    arc.Load(arcStream);
    std::unordered_map<std::string, ARC_Entry const&> mapNameEntry;

    for (ARC_Entry const& entry : arc.entries)
    {
        if (entry.ext == ARC_ExtensionHash::GMD)
        {
            std::string name = "gmd__" + ConvertToID(entry.filename);
            if (mapNameEntry.contains(name))
                throw runtime_error("Duplicated name '{}' in {}", name);
            mapNameEntry.insert({name, entry});
        }
    }

    // 2. Prepare workspace

    if (mapNameEntry.size() == 0)
        return;

    _ensureEmptyFolder(destFolder);

    // 3. Do actual work

    for (auto const& [name, entry] : mapNameEntry)
    {
        std::string s = entry.Decompress();
        stream_ptr gmdStream{name, s};
        TGAAC_ExtractGMD(gmdStream, destFolder / name);
    }

    // 4. Write metadata

    using namespace rapidjson;
    FILE* f = fopen((destFolder / "__META__.json").c_str(), "wb");
    if (f == nullptr)
        throw runtime_error("Could not open file META.json");

    char writeBuffer[65536];
    FileWriteStream os{f, writeBuffer, sizeof(writeBuffer)};
    PrettyWriter<FileWriteStream> json{os};

    json.StartObject();
    json.Key("version");
    json.Int64(arc.version);
    json.Key("entries");
    json.StartObject();
    for (auto& [name, entry] : mapNameEntry)
    {
        json.Key(name.c_str());
        json.StartObject();
        json.Key("filename");
        json.String(entry.filename.c_str());
        json.Key("ext");
        json.Int64((uint32_t)entry.ext);
        json.Key("wasCompressed");
        json.Bool(entry.content.size() != entry.decompSize);
        json.EndObject();
    }
    json.EndObject();
    json.EndObject();
    fclose(f);
}

void TGAAC_GlobalExtract(fs::path const& installFolder, fs::path const& extractFolder)
{
    _ensureEmptyFolder(extractFolder);

    std::unordered_map<std::string, fs::path> mapNamePath;

    mapNamePath.reserve(100);
    for (fs::path const& p : fs::recursive_directory_iterator(installFolder))
    {
        if (p.extension() != ".arc")
            continue;
        fs::path arcPath = fs::relative(p, installFolder);
        mapNamePath.emplace(ConvertToID(arcPath.string()), arcPath);
    }

    fmt::print("Found {} ARC files\n", mapNamePath.size());

    for (auto& [name, arcPath] : mapNamePath)
    {
        fmt::print("Extracting {}...\n", name);
        stream_ptr arcStream{installFolder / arcPath};
        TGAAC_ExtractARC(arcStream, extractFolder / name);
    }

    using namespace rapidjson;
    FILE* f = fopen((extractFolder / "__META__.json").c_str(), "wb");
    if (f == nullptr)
        throw runtime_error("Could not open file META.json");

    char writeBuffer[65536];
    FileWriteStream os{f, writeBuffer, sizeof(writeBuffer)};
    PrettyWriter<FileWriteStream> json{os};

    json.StartObject();
    json.Key("entries");
    json.StartObject();
    for (auto& [name, arcPath] : mapNamePath)
    {
        json.Key(name.c_str());
        json.String(arcPath.c_str());
    }
    json.EndObject();
    json.EndObject();
    fclose(f);
}
