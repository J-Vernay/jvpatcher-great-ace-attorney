// TGAAC_jv_patcher : Extract and modify scripts of The Great Ace Attorney Chronicles.
// Copyright (C) 2023 Julien Vernay - Available as GNU GPL-3.0-or-later

#include "Utility.hpp"
#include <cctype>

String ConvertToID(StringView input)
{
    String output;
    output.reserve(input.size());

    // Replace non-alnum chars by hyphen, only one consecutive.
    char last = '-';
    for (char c : input)
    {
        if (!std::isalnum(c))
            c = '-';
        if (c == last && c == '-')
            continue;
        output += c;
        last = c;
    }

    return output;
}

Stream::Stream(Path const& path)
{
    name = path.filename().string();
    file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
        throw RuntimeError("Could not open {}", path.c_str());
}

Stream::Stream(String name_, VecByte content_)
{
    name = std::move(name_);
    file = nullptr;
    content = std::move(content_);
    cursor = 0;
}

Stream::~Stream() noexcept
{
    if (file)
        std::fclose(file);
}

void Stream::Seek(int64_t offset, int whence)
{
    if (file)
    {
        int res = fseek(file, offset, whence);
        if (res != 0)
            throw RuntimeError("Invalid fseek():  errno={}", res, errno);
    }
    else
    {
        if (whence == SEEK_SET)
            cursor = std::clamp<int64_t>(0, offset, content.size());
        else if (whence == SEEK_CUR)
            cursor = std::clamp<int64_t>(0, cursor + offset, content.size());
        else if (whence == SEEK_END)
            cursor = std::clamp<int64_t>(0, content.size() + offset, content.size());
        else
            throw RuntimeError("Invalid whence={} for {}", whence, name);
    }
}

int64_t Stream::Tell()
{
    if (file)
    {
        long res = std::ftell(file);
        if (res < 0)
            throw RuntimeError("Bad ftell(): {} errno={}", res, errno);
        return (int64_t)res;
    }
    else
    {
        return cursor;
    }
}

void Stream::ReadBytes(SpanByte out)
{
    if (file)
    {
        int64_t result = std::fread(out.data(), 1, out.size(), file);
        if (result != out.size())
            throw RuntimeError("Could not read {} in {}", out.size(), name);
    }
    else
    {
        if (cursor + out.size_bytes() > content.size())
            throw RuntimeError("Not enough bytes in stream {}", name);
        std::memcpy(out.data(), content.data() + cursor, out.size_bytes());
        cursor += out.size_bytes();
    }
}

void Stream::ReadCStr(VecByte& out, std::byte delimiter)
{
    if (file)
    {
        while (true)
        {
            std::byte buffer[1024];
            int64_t count = std::fread(buffer, 1, sizeof(buffer), file);
            void* p = std::memchr(buffer, (int)delimiter, count);
            if (p)
            {
                int64_t len = (std::byte*)p - buffer + 1;
                out.insert(out.end(), buffer, buffer + len);
                Seek(len - count, SEEK_CUR);
                return;
            }
            else if (count == sizeof(buffer))
            {
                out.insert(out.end(), buffer, buffer + count);
            }
            else
            {
                throw RuntimeError(
                    "{}: ReadCStr delimiter {} not found", name, delimiter);
            }
        }
    }
    else
    {
        std::byte* baseptr = content.data() + cursor;
        void* p = std::memchr(baseptr, (int)delimiter, content.size() - cursor);
        if (p)
        {
            int64_t len = (std::byte*)p - baseptr + 1;
            out.insert(out.end(), baseptr, baseptr + len);
            cursor += len;
        }
        else
        {
            throw RuntimeError("{}: ReadCStr delimiter {} not found", name, delimiter);
        }
    }
}

String const& Stream::Name() const noexcept
{
    return name;
}
