# TGAAC_jv_patcher

A fanwork to extract and modify the script of The Great Ace Attorney Chronicles (TGAAC).
This is aimed to help translation of this great game!

## How to run

For now, this is still a prototype. You will need a C++ compiler and CMake.

```
cmake . -B build
cmake --build build
./build/TGAAC_jv_patcher <archive_folder> <extract_folder>
```
With:
- `archive_folder` is the archive folder found in the game files.
  For instance: `~/.local/share/Steam/steamapps/common/TGAAC/nativeDX11x64/archive`
- `extract_folder` is the destination of all extracted files.


## Credits / Attributions

This work could not have been done without the preliminary
retro-engineering work for Kuriimu 1 & 2, especially:
- ARC: https://github.com/FanTranslatorsInternational/Kuriimu2/tree/dev/plugins/Capcom/plugin_mt_framework/Archives
- GMD: https://github.com/IcySon55/Kuriimu/tree/master/src/text/text_gmd

And without the following free (as in freedom) libraries:
- fmtlib : https://github.com/fmtlib/fmt  
  Under MIT license:  
  Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
- rapidjson : https://rapidjson.org/  
  Under MIT license:
  Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.
