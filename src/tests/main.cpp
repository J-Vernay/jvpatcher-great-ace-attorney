// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "../TGAAC_file_ARC.hpp"
#include "../TGAAC_file_GMD.hpp"
#include "../Utility.hpp"
#include <filesystem>

void test_ARC_Archive(stream_ptr& arcStream);
void test_GMD_Archive(stream_ptr& gmdStream);

// For debugging purposes
// fs::path const TGAAC_DIR = "~/.local/share/Steam/steamapps/common/TGAAC";
// fs::path const ARCHIVE_DIR = TGAAC_DIR / "nativeDX11x64/archive";

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fmt::print("Usage: {} <archive_folder>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fs::path archiveFolder = argv[1];
    int nbErrors = 0;
    int nbTests = 0;
    for (fs::path const& p : fs::recursive_directory_iterator(archiveFolder))
    {
        if (p.extension() != ".arc")
            continue;
        try
        {
            stream_ptr arcStream{p};
            test_ARC_Archive(arcStream);
        }
        catch (std::exception const& e)
        {
            fmt::print("ERROR with {} - {}\n", fs::relative(p, archiveFolder).string(),
                       e.what());
            ++nbErrors;
            ++nbTests;
        }
    }
    fmt::print("NB_ERRORS: {}\n", nbErrors);
    return EXIT_SUCCESS;
}

void test_ARC_Archive(stream_ptr& arcStream)
{
    ARC_Archive arc;
    arc.Load(arcStream);

    for (ARC_Entry const& entry : arc.entries)
    {
        if (entry.ext == ARC_ExtensionHash::GMD)
        {
            fmt::print("Testing {} / {}...\n", arcStream.Name(), entry.filename);
            stream_ptr gmdStream{entry.filename, entry.Decompress()};
            test_GMD_Archive(gmdStream);
        }
    }
}

void test_GMD_Archive(stream_ptr& gmdStream)
{
    size_t fileSize = gmdStream.SeekInput(0, std::ios::end);
    gmdStream.SeekInput(0, std::ios::beg);

    auto inputStorage = std::make_unique_for_overwrite<uint8_t[]>(fileSize);
    std::span<uint8_t> inputBytes{inputStorage.get(), fileSize};
    gmdStream.Read(inputBytes);
    gmdStream.SeekInput(0, std::ios::beg);

    GMD_Registry gmd;
    gmd.Load(gmdStream);

    stream_ptr gmdOut{fmt::format("out--{}", gmdStream.Name()), std::string{}};
    gmd.Save(gmdOut);
    std::string_view view = dynamic_cast<std::stringbuf&>(*gmdOut.get()).view();
    std::span<uint8_t> outputBytes{(uint8_t*)view.data(), view.size()};

    GMD_Registry gmd2;
    gmd2.Load(gmdOut);

    stream_ptr{"in.bin", std::ios::out | std::ios::trunc}.Write(inputBytes);
    stream_ptr{"out.bin", std::ios::out | std::ios::trunc}.Write(outputBytes);

    if (inputBytes.size() != outputBytes.size())
        gmdOut.Error("size mismatch, in={} out={}", inputBytes.size(),
                     outputBytes.size());

    auto it = std::ranges::mismatch(inputBytes, outputBytes);
    int64_t pos = it.in1 - inputBytes.begin();
    if (pos == 12)
    {
        // Range [12;20[ is padding: mismatch is OK
        it = std::ranges::mismatch(inputBytes.subspan(pos + 8),
                                   outputBytes.subspan(pos + 8));
        pos = it.in1 - inputBytes.begin();
    }

    if (pos < inputBytes.size())
    {
        gmdOut.Error("mismatch at {}:\n\tin ={}\n\tout={}", pos,
                     fmt::join(inputBytes.subspan(pos, 10), ","),
                     fmt::join(outputBytes.subspan(pos, 10), ","));
    }
}