//----------------------------------------------------------------------------------------------------
// FileUtils.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <filesystem>
#include <vector>

#include "Engine/Core/StringUtils.hpp"

//----------------------------------------------------------------------------------------------------
// Basic file I/O operations
bool FileReadToBuffer(std::vector<uint8_t>& out_buffer, String const& fileName);
bool FileReadToString(String& out_string, String const& fileName);
bool FileWriteFromBuffer(std::vector<uint8_t> const& buffer, String const& fileName);
bool FileWriteBinary(String const& fileName, void const* data, size_t dataSize);

//----------------------------------------------------------------------------------------------------
// Directory utilities
bool EnsureDirectoryExists(String const& directoryPath);

//----------------------------------------------------------------------------------------------------
// RLE Compression utilities
template <typename T>
struct sRLEEntry
{
    T       value;
    uint8_t count;
};

template <typename T>
std::vector<sRLEEntry<T>> CompressRLE(std::vector<T> const& data)
{
    std::vector<sRLEEntry<T>> result;

    if (data.empty()) return result;

    T       currentValue = data[0];
    uint8_t count        = 1;

    for (size_t i = 1; i < data.size(); ++i)
    {
        if (data[i] == currentValue && count < 255)
        {
            count++;
        }
        else
        {
            // End current run
            result.push_back({currentValue, count});
            currentValue = data[i];
            count        = 1;
        }
    }

    // Don't forget the last run
    result.push_back({currentValue, count});
    return result;
}

//----------------------------------------------------------------------------------------------------
template <typename T>
bool DecompressRLE(std::vector<sRLEEntry<T>> const& entries,
                   std::vector<T>&                  output,
                   size_t                           expectedSize)
{
    output.clear();
    output.reserve(expectedSize);

    for (sRLEEntry<T> const& entry : entries)
    {
        for (int i = 0; i < entry.count; ++i)
        {
            output.push_back(entry.value);
            if (output.size() > expectedSize)
            {
                return false; // Too much data
            }
        }
    }

    return output.size() == expectedSize;
}
