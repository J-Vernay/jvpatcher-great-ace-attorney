// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#ifndef JV_TGAAC_FILE_GMD_H
#define JV_TGAAC_FILE_GMD_H

#include "Utility.hpp"

struct GMD_Entry
{
    std::string key;
    std::string value;
    uint32_t hash1{}; ///< Some hash based on 'key'
    uint32_t hash2{}; ///< Some other hash based on 'key'
};

struct GMD_Registry
{
    uint32_t version;
    uint32_t language;
    std::string name;
    std::vector<GMD_Entry> entries;
};

GMD_Registry GMD_Load(stream_ptr& gmd);

/// Modifies the given GMD to make edition more easier:
/// - Line breaks are made insignificant.
/// - Long event sequences <E123><E456> are converted to <JV123>
std::string GMD_EscapeEntryJV(std::string_view input);

#endif
