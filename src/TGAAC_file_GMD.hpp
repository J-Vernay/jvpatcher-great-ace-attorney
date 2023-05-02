// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#ifndef JV_TGAAC_FILE_GMD_H
#define JV_TGAAC_FILE_GMD_H

#include "Utility.hpp"

#include <map>

struct GMD_Entry
{
    std::string key;
    std::string value;
};

struct GMD_Registry
{
    uint32_t version;
    uint32_t language;
    std::string name;
    std::vector<GMD_Entry> entries;

    void Load(stream_ptr& in);
    void Save(stream_ptr& out) const;
    void ReadFiles(fs::path const& inFolder);
    void WriteFiles(fs::path const& outFolder) const;
};

/// Modifies the given GMD to make edition more easier:
/// - Line breaks are made insignificant.
/// - Long event sequences <E123><E456> are converted to <JV123>
std::string GMD_EscapeEntryJV(std::string_view input);

/// Reverts the operation of GMD_EscapeEntryJV
std::string GMD_UnescapeEntryJV(std::string_view input);

#endif
