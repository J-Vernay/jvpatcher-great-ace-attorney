#ifndef JV_TGAAC_UTILITY_HPP
#define JV_TGAAC_UTILITY_HPP

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

using String = std::string;
using VecByte = std::vector<std::byte>;
using SpanByte = std::span<std::byte>;
using Path = std::filesystem::path;
template <typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

/// Removes directory separators and whitespaces.
String ConvertToID(String const& input);

/// Used extensively for errors.
struct RuntimeError : std::runtime_error
{
    template <typename S, typename... TArgs>
    explicit RuntimeError(S const& format, TArgs&&... args);
};

/// Mimics a file, either backed by FILE* or by a memory buffer.
class Stream
{
  public:
    explicit Stream(Path const& path);
    explicit Stream(String name, VecByte content);
    ~Stream() noexcept;

    void Seek(int64_t offset, int whence);
    int64_t Tell();
    template <typename T>
    void Read(std::span<T> out);
    void ReadBytes(SpanByte out);
    void ReadCStr(VecByte& out, std::byte delimiter = std::byte{0});

    String const& Name() const noexcept;

    // No copy, no move
    Stream(Stream const&) = delete;
    Stream(Stream&&) = delete;
    Stream& operator=(Stream const&) = delete;
    Stream& operator=(Stream&&) = delete;

  private:
    String name;
    std::FILE* file = nullptr; ///< If null, use 'content' instead
    VecByte content;
    int64_t cursor;
};

//
// ==================== IMPLEMENTATION ====================
//

template <typename S, typename... TArgs>
inline RuntimeError::RuntimeError(S const& format, TArgs&&... args)
    : runtime_error(fmt::vformat(format, fmt::make_format_args(args...)))
{
}

template <typename T>
inline void Stream::Read(std::span<T> out)
{
    static_assert(std::is_trivial_v<T>);
    ReadBytes(SpanByte{(std::byte*)out.data(), out.size_bytes()});
}

#endif