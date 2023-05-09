// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#ifndef JV_TGAAC_ACTIONS_H
#define JV_TGAAC_ACTIONS_H

/// This file contains the global actions done on the complete installation path.

#include <functional>

#include "Utility.hpp"

struct GMD_Registry;
struct ARC_Archive;

/// Serialize assets content on filesystem as separate files,
/// to make editing easier and conflict-less.
/// Note that only supported entries (which have an ARC_ExtensionHash
/// enumerant value) will be extracted on filesystem.

void TGAAC_WriteFolder_ARC(ARC_Archive const& arc, fs::path const& outFolder);
void TGAAC_WriteFolder_GMD(GMD_Registry const& gmd, fs::path const& outFolder);

void TGAAC_ReadFolder_ARC(ARC_Archive& arc, fs::path const& inFolder);
void TGAAC_ReadFolder_GMD(GMD_Registry& gmd, fs::path const& inFolder);

void TGAAC_GlobalExtract(fs::path const& installFolder, fs::path const& extractFolder);

#endif