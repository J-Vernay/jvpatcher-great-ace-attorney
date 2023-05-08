// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#ifndef JV_TGAAC_UTILITY_HPP
#define JV_TGAAC_UTILITY_HPP

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <pugixml.hpp>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

#include <archive_crc32.h> // crc32(seed, data, size)

namespace fs = std::filesystem;

/// Used extensively for errors.
struct runtime_error : std::runtime_error
{
    template <typename S, typename... TArgs>
    explicit runtime_error(S const& format, TArgs const&... args);
};

class stream_ptr : public std::unique_ptr<std::streambuf>
{
    std::string m_name;

  public:
    stream_ptr(fs::path const& p, std::ios::openmode mode = std::ios::in);
    stream_ptr(std::string name, std::string bytes);
    std::string_view Name() const noexcept;

    int64_t SeekInput(int64_t off, std::ios::seekdir);
    int64_t SeekOutput(int64_t off, std::ios::seekdir);

    template <typename T>
    void Read(std::span<T> out);
    template <typename T>
    void Write(std::span<T> in);

    void Sync();

    std::string ReadCStr();
    std::string ReadAll();

    /// Concise unconditional throw.
    template <typename S, typename... TArgs>
    [[noreturn]] void Error(S const& format, TArgs const&... args);
};

/// Removes directory separators and whitespaces.
std::string ConvertToID(std::string_view input);

/// Throw an exception if given path is not an empty directory.
void EnsureEmptyDirectory(fs::path const& folder);

//
// ==================== IMPLEMENTATION ====================
//

template <typename S, typename... TArgs>
inline runtime_error::runtime_error(S const& format, TArgs const&... args)
    : std::runtime_error(fmt::vformat(format, fmt::make_format_args(args...)))
{
}

template <typename T>
void stream_ptr::Read(std::span<T> out)
{
    static_assert(std::is_trivial_v<T>);
    int64_t result = get()->sgetn((char*)out.data(), out.size_bytes());
    if (result != out.size_bytes())
        Error("Could not read {} bytes in {}", out.size_bytes(), m_name);
}

template <typename T>
void stream_ptr::Write(std::span<T> in)
{
    static_assert(std::is_trivial_v<T>);
    int64_t result = get()->sputn((char const*)in.data(), in.size_bytes());
    if (result != in.size_bytes())
        Error("Could not write {} bytes in {}", in.size_bytes(), m_name);
}

template <typename S, typename... TArgs>
[[noreturn]] void stream_ptr::Error(S const& format, TArgs const&... args)
{
    throw runtime_error(fmt::format("{{}}: {}", format), m_name, args...);
}
#endif