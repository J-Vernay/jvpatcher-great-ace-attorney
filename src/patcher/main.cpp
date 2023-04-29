// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "../TGAAC_actions.hpp"
#include "../Utility.hpp"
#include <filesystem>

// For debugging purposes
// fs::path const TGAAC_DIR = "~/.local/share/Steam/steamapps/common/TGAAC";
// fs::path const ARCHIVE_DIR = TGAAC_DIR / "nativeDX11x64/archive";

char const* SHORT_LICENSE = R"(
    TGAAC_jv_patcher  Copyright (C) 2023  Julien Vernay
    This program comes with ABSOLUTELY NO WARRANTY.
    This is free software, and you are welcome to redistribute it under certain conditions.
    Use "--license" argument fore more details.

)";

char const* LONG_LICENSE = R"(
    TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
    Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

)";

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "--license") == 0)
    {
        fmt::print("{}", LONG_LICENSE);
        return EXIT_SUCCESS;
    }

    fmt::print("{}", SHORT_LICENSE);

    if (argc != 3)
    {
        fmt::print("Usage: {} <archive_folder> <extract_folder>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fs::path archiveFolder = argv[1];
    fs::path extractFolder = argv[2];

    fmt::print("Decompressing {} into {}", archiveFolder.c_str(), extractFolder.c_str());

    fs::remove_all(extractFolder);
    TGAAC_GlobalExtract(archiveFolder, extractFolder);

    return EXIT_SUCCESS;
}
