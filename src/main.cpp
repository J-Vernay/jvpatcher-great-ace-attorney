
#include "TGAAC_actions.hpp"
#include "Utility.hpp"
#include <filesystem>

// For debugging purposes
// Path const TGAAC_DIR = "~/.local/share/Steam/steamapps/common/TGAAC";
// Path const ARCHIVE_DIR = TGAAC_DIR / "nativeDX11x64/archive";

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        fmt::print("Usage: {} <archive_folder> <extract_folder>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Path archiveFolder = argv[1];
    Path extractFolder = argv[2];

    std::filesystem::remove_all(extractFolder);
    TGAAC_GlobalExtract(archiveFolder, extractFolder);
}
