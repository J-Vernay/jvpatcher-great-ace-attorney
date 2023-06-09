// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "Utility.hpp"
#include <cctype>
#include <filesystem>
#include <fstream>

using namespace std;

std::string ConvertToID(std::string_view input)
{
    std::string output;
    output.reserve(input.size());

    // Replace non-alnum chars by hyphen, only one consecutive.
    char8_t last = '-';
    for (char8_t c : input)
    {
        if (!isalnum(c))
            c = '-';
        if (c == last && c == '-')
            continue;
        output += c;
        last = c;
    }

    return output;
}

stream_ptr::stream_ptr(fs::path const& p, std::ios::openmode mode)
    : unique_ptr{make_unique<filebuf>()}, m_name{p.filename()}
{
    dynamic_cast<filebuf*>(get())->open(p, mode | ios::binary);
}

stream_ptr::stream_ptr(std::string name, std::string bytes)
    : unique_ptr{make_unique<stringbuf>(std::move(bytes))}, m_name{std::move(name)}
{
}

int64_t stream_ptr::SeekInput(int64_t off, std::ios::seekdir seekdir)
{
    return get()->pubseekoff(off, seekdir, std::ios::in);
}

int64_t stream_ptr::SeekOutput(int64_t off, std::ios::seekdir seekdir)
{
    return get()->pubseekoff(off, seekdir, std::ios::out);
}

std::string_view stream_ptr::Name() const noexcept
{
    return m_name;
}

void stream_ptr::Sync()
{
    get()->pubsync();
}

std::string stream_ptr::ReadCStr()
{
    std::string str;
    while (true)
    {
        char c = get()->sbumpc();
        if (c == '\0' || c == std::streambuf::traits_type::eof())
            return str;
        str.push_back(c);
    }
}

std::string stream_ptr::ReadAll()
{
    std::string result;
    int64_t originalPos = SeekInput(0, std::ios::cur);
    int64_t fileSize = SeekInput(0, std::ios::end);
    result.resize(fileSize);
    SeekInput(0, std::ios::beg);
    Read(std::span{result});
    SeekInput(originalPos, std::ios::beg);
    return result;
}

void CreateEmptyDirectory(fs::path const& folder)
{
    if (!fs::exists(folder))
        fs::create_directories(folder);
    for (fs::path const& entry : fs::directory_iterator(folder))
        throw ::runtime_error("Not an empty directory, found {} in {}",
                              entry.filename().string(), folder.string());
}