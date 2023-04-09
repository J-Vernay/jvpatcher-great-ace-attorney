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

