// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#ifndef JV_TGAAC_ACTIONS_H
#define JV_TGAAC_ACTIONS_H

/// This file contains the global actions done on the complete installation path.

#include <functional>

#include "Utility.hpp"

// 'destFolder' must be empty or non-existent.

void TGAAC_ExtractGMD(Stream& gmdStream, Path const& destFolder);
void TGAAC_ExtractARC(Stream& arcStream, Path const& destFolder);
void TGAAC_GlobalExtract(Path const& installFolder, Path const& extractFolder);

#endif