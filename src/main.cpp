
#include "TGAAC_files.hpp"
#include <algorithm>
#include <fstream>
#include <string_view>

Path const TGAAC_DIR = "/home/jvernay/.local/share/Steam/steamapps/common/TGAAC";
Path const ARCHIVE_DIR = TGAAC_DIR / "nativeDX11x64/archive";

void ParseGMD(VecByte b);

int main()
{
    Stream streamARC{ARCHIVE_DIR / "title_select_eng.arc"};
    ARC_Archive arc = ARC_LoadFromFile(streamARC);
    for (ARC_Entry& entry : arc.entries)
    {
        if (entry.ext == ARC_ExtensionHash::GMD)
        {
            VecByte content = ARC_DecompressEntry(entry);
            std::string_view s{(char*)content.data(), content.size()};

            std::ofstream{"out.bin", std::ios::binary}.write(
                (char*)content.data(), content.size());

            Stream gmd{entry.filename, std::move(content)};
            GMD_LoadFromFile(gmd);
        }
    }
}
