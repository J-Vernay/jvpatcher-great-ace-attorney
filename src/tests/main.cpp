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

    bool CheckMismatch(std::span<uint8_t const> a, std::span<uint8_t const> b)
    {
        int64_t pos = 0;
        bool wasError = false;
        // FOR DEBUG
        // stream_ptr{"../a.bin", std::ios::out}.Write(a);
        // stream_ptr{"../b.bin", std::ios::out}.Write(b);
        while (true)
        {
            Check(a.size() == b.size(), "size mismatch, in={} out={}\n", a.size(),
                  b.size());

            std::ranges::in_in_result it =
                std::ranges::mismatch(a.subspan(pos), b.subspan(pos));

            pos = it.in1 - a.begin();
            if (pos == a.size())
                break; // done, no mismatch

            Check(false, "mismatch at 0x{:X}:\n\tin =0x{:02X}\n\tout=0x{:02X}\n", pos,
                  fmt::join(a.subspan(pos, 10), "'"), fmt::join(b.subspan(pos, 10), "'"));
            pos += 8;
            wasError = true;
        }
        return wasError;
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

    fmt::print("Testing {}\n", arcStream.Name());

    // Check ARC Save/Load identity

    std::string inputStorage = arcStream.ReadAll();
    std::span<uint8_t> inputBytes{(uint8_t*)inputStorage.data(), inputStorage.size()};

    stream_ptr arcOut{fmt::format("out--{}", arcStream.Name()), std::string{}};
    arc.Save(arcOut);
    std::string_view view = dynamic_cast<std::stringbuf&>(*arcOut.get()).view();
    std::span<uint8_t> outputBytes{(uint8_t*)view.data(), view.size()};

    T.CheckMismatch(inputBytes, outputBytes);

    // Check GMD

    for (ARC_Entry const& entry : arc.entries)
    {
        if (entry.ext == ARC_ExtensionHash::GMD)
        {
            fmt::print("Testing {} / {}...\n", arcStream.Name(), entry.filename);
            stream_ptr gmdStream{entry.filename,
                                 ARC_Entry::Decompress(entry.content, entry.decompSize)};
            test_GMD_Archive(T, gmdStream);
        }
    }
}

void test_GMD_Archive(TestCase& T, stream_ptr& gmdStream)
{
    std::string inputStorage = gmdStream.ReadAll();
    std::span<uint8_t> inputBytes{(uint8_t*)inputStorage.data(), inputStorage.size()};

    GMD_Registry gmd;
    gmd.Load(gmdStream);

    stream_ptr gmdOut{fmt::format("out--{}", gmdStream.Name()), std::string{}};
    gmd.Save(gmdOut);
    std::string_view view = dynamic_cast<std::stringbuf&>(*gmdOut.get()).view();
    std::span<uint8_t> outputBytes{(uint8_t*)view.data(), view.size()};

    T.CheckMismatch(inputBytes, outputBytes);
}