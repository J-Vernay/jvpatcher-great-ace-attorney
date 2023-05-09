// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "TGAAC_actions.hpp"
#include "TGAAC_file_ARC.hpp"
#include "TGAAC_file_GMD.hpp"

static constexpr std::string_view META_FILE = "__meta__.xml";

void TGAAC_WriteFolder_ARC(ARC_Archive const& arc, fs::path const& outFolder)
{
    CreateEmptyDirectory(outFolder);

    pugi::xml_document xmlMeta;
    pugi::xml_node xmlRoot = xmlMeta.append_child("ARC_Archive");

    xmlRoot.append_child("version").text().set(arc.version);
    xmlRoot.append_child("hasExtendedNames").text().set(arc.hasExtendedNames);

    pugi::xml_node xmlEntries = xmlRoot.append_child("entries");

    for (ARC_Entry const& entry : arc.entries)
    {
        if (entry.ext != ARC_ExtensionHash::GMD)
            continue;

        std::string entryFolder = "gmd__" + ConvertToID(entry.filename);
        pugi::xml_node xmlEntry = xmlEntries.append_child("GMD_Entry");
        xmlEntry.append_attribute("key").set_value(entry.filename.c_str());
        xmlEntry.append_attribute("file").set_value(entryFolder.c_str());
        xmlEntry.append_child("ext").text().set((uint32_t)entry.ext);
        xmlEntry.append_child("isCompressed").text().set(entry.isCompressed);
        xmlEntry.append_child("unknownFlags").text().set(entry.unknownFlags);

        std::string gmdBytes =
            entry.isCompressed ? ARC_Entry::Decompress(entry.content, entry.decompSize)
                               : entry.content;
        stream_ptr gmdStream{entry.filename, gmdBytes};
        GMD_Registry gmd;
        gmd.Load(gmdStream);
        TGAAC_WriteFolder_GMD(gmd, outFolder / entryFolder);
    }

    xmlMeta.save_file((outFolder / META_FILE).string().c_str());
}

void TGAAC_WriteFolder_GMD(GMD_Registry const& gmd, fs::path const& outFolder)
{
    CreateEmptyDirectory(outFolder);

    // Create metafile, storing Registry infos which are not part of entries.
    pugi::xml_document xmlMeta;
    pugi::xml_node xmlRoot = xmlMeta.append_child("GMD_Registry");

    xmlRoot.append_child("version").text().set(gmd.version);
    xmlRoot.append_child("language").text().set(gmd.language);
    xmlRoot.append_child("name").text().set(gmd.name.c_str());
    xmlRoot.append_child("_padding").text().set(gmd._padding);

    pugi::xml_node xmlEntries = xmlRoot.append_child("entries");
    for (GMD_Entry const& entry : gmd.entries)
    {
        std::string entryFilename = ConvertToID(entry.key) + ".txt";
        pugi::xml_node xmlEntry = xmlEntries.append_child("GMD_Entry");
        xmlEntry.append_attribute("key").set_value(entry.key.c_str());
        xmlEntry.append_attribute("file").set_value(entryFilename.c_str());

        stream_ptr{outFolder / entryFilename, std::ios::out}.Write(
            std::span{entry.value});
    }

    xmlMeta.save_file((outFolder / META_FILE).string().c_str());
}

void TGAAC_ReadFolder_ARC(ARC_Archive& arc, fs::path const& inFolder)
{
    arc = {};

    pugi::xml_document xmlMeta;
    pugi::xml_parse_result result =
        xmlMeta.load_file((inFolder / META_FILE).string().c_str());

    if (!result)
        throw runtime_error("ARC meta error: {} at {}", result.description(),
                            result.offset);

    pugi::xml_node xmlRoot = xmlMeta.child("ARC_Archive");
    arc.version = xmlRoot.child("version").text().as_ullong();
    arc.hasExtendedNames = xmlRoot.child("hasExtendedNames").text().as_bool();

    pugi::xml_node xmlEntries = xmlRoot.child("entries");
    for (auto xmlEntry = xmlEntries.first_child(); xmlEntry;
         xmlEntry = xmlEntry.next_sibling())
    {
        auto& entry = arc.entries.emplace_back();

        entry.filename = xmlEntry.attribute("key").value();
        std::string entryFolder = xmlEntry.attribute("file").value();
        entry.ext = (ARC_ExtensionHash)xmlEntry.child("ext").text().as_ullong();
        entry.isCompressed = xmlEntry.child("isCompressed").text().as_bool();
        entry.unknownFlags = xmlEntry.child("unknownFlags").text().as_ullong();

        if (entry.ext != ARC_ExtensionHash::GMD)
            throw runtime_error("Unsupported entry extension {}", (uint32_t)entry.ext);

        GMD_Registry gmd;
        TGAAC_ReadFolder_GMD(gmd, inFolder / entryFolder);
        stream_ptr gmdOut = {entry.filename, std::string{}};
        gmd.Save(gmdOut);

        std::string_view gmdBytes = dynamic_cast<std::stringbuf&>(*gmdOut.get()).view();
        entry.decompSize = gmdBytes.size();

        if (entry.isCompressed)
            entry.content = ARC_Entry::Compress(gmdBytes);
        else
            entry.content = gmdBytes;
    }
}

void TGAAC_ReadFolder_GMD(GMD_Registry& gmd, fs::path const& inFolder)
{
    gmd = {};

    // Create metafile, storing Registry infos which are not part of entries.
    pugi::xml_document xmlMeta;
    pugi::xml_parse_result result =
        xmlMeta.load_file((inFolder / META_FILE).string().c_str());

    if (!result)
        throw runtime_error("GMD meta error: {} at {}", result.description(),
                            result.offset);

    pugi::xml_node xmlRoot = xmlMeta.child("GMD_Registry");

    gmd.version = xmlRoot.child("version").text().as_ullong();
    gmd.language = xmlRoot.child("language").text().as_ullong();
    gmd.name = xmlRoot.child("name").text().as_string();
    gmd._padding = xmlRoot.child("_padding").text().as_ullong();

    pugi::xml_node xmlEntries = xmlRoot.child("entries");
    for (auto xmlEntry = xmlEntries.first_child(); xmlEntry;
         xmlEntry = xmlEntry.next_sibling())
    {
        std::string key = xmlEntry.attribute("key").value();
        std::string entryFilename = xmlEntry.attribute("file").value();

        auto& entry = gmd.entries.emplace_back();
        entry.key = std::move(key);
        entry.value = stream_ptr{inFolder / entryFilename}.ReadAll();
    }
}

void TGAAC_GlobalExtract(fs::path const& installFolder, fs::path const& extractFolder)
{
    CreateEmptyDirectory(extractFolder);

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
        ARC_Archive arc;
        arc.Load(arcStream);
        TGAAC_WriteFolder_ARC(arc, extractFolder / name);
    }

    throw runtime_error("TODO: __meta__.xml");
}
