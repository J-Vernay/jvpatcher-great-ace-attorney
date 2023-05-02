// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "../TGAAC_file_ARC.hpp"
#include "../TGAAC_file_GMD.hpp"
#include "../Utility.hpp"
#include <bits/ranges_util.h>
#include <filesystem>

// For debugging purposes
// fs::path const TGAAC_DIR = "~/.local/share/Steam/steamapps/common/TGAAC";
// fs::path const ARCHIVE_DIR = TGAAC_DIR / "nativeDX11x64/archive";

struct TestCase
{
    int32_t nbChecks{};
    int32_t nbFailures{};

    template <typename S, typename... TArgs>
    bool Check(bool cond, S const& format, TArgs const&... args)
    {
        ++nbChecks;
        if (cond)
            return true;
        ++nbFailures;
        fmt::vprint(format, fmt::make_format_args(args...));
        return false;
    }

    template <typename S, typename... TArgs>
    void Require(bool cond, S const& format, TArgs const&... args)
    {
        ++nbChecks;
        if (cond)
            return;
        ++nbFailures;
        fmt::vprint(format, fmt::make_format_args(args...));
        throw *this;
    }
};

void test_ARC_Archive(TestCase& T, stream_ptr& arcStream);
void test_GMD_Archive(TestCase& T, stream_ptr& gmdStream);

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        fmt::print("Usage: {} <archive_folder>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fs::path archiveFolder = argv[1];
    TestCase T;
    for (fs::path const& p : fs::recursive_directory_iterator(archiveFolder))
    {
        if (p.extension() != ".arc")
            continue;
        try
        {
            stream_ptr arcStream{p};
            test_ARC_Archive(T, arcStream);
        }
        catch (TestCase&)
        {
            fmt::print("ERROR with {}\n", fs::relative(p, archiveFolder).string());
        }
    }
    fmt::print("nbChecks   = {}\n", T.nbChecks);
    fmt::print("nbFailures = {}\n", T.nbFailures);
    return EXIT_SUCCESS;
}

void test_ARC_Archive(TestCase& T, stream_ptr& arcStream)
{
    ARC_Archive arc;
    arc.Load(arcStream);

    for (ARC_Entry const& entry : arc.entries)
    {
        if (entry.ext == ARC_ExtensionHash::GMD)
        {
            fmt::print("Testing {} / {}...\n", arcStream.Name(), entry.filename);
            stream_ptr gmdStream{entry.filename, entry.Decompress()};
            test_GMD_Archive(T, gmdStream);
        }
    }
}

void test_GMD_Archive(TestCase& T, stream_ptr& gmdStream)
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

    T.Check(inputBytes.size() == outputBytes.size(), //
            "size mismatch, in={} out={}", inputBytes.size(), outputBytes.size());

    int64_t pos = 0;
    while (true)
    {
        std::ranges::in_in_result it =
            std::ranges::mismatch(inputBytes.subspan(pos), outputBytes.subspan(pos));

        pos = it.in1 - inputBytes.begin();
        if (pos == inputBytes.size())
            break; // done, no mismatch

        T.Check(false, "mismatch at 0x{:X}:\n\tin ={}\n\tout={}\n", pos,
                fmt::join(inputBytes.subspan(pos, 10), ","),
                fmt::join(outputBytes.subspan(pos, 10), ","));
        pos += 8;
    }
}