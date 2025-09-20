//----------------------------------------------------------------------------------------------------
// FileUtils.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

//----------------------------------------------------------------------------------------------------
bool FileReadToBuffer(std::vector<uint8_t>& out_buffer, String const& fileName)
{
    FILE*         file = nullptr;
    errno_t const err  = fopen_s(&file, fileName.c_str(), "rb");

    if (err != 0 || !file)
    {
        ERROR_RECOVERABLE("Error: Failed to open file.")
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        ERROR_RECOVERABLE("Error: Failed to seek to end of file.")
        fclose(file);
        return false;
    }

    long const fileSize = ftell(file);

    if (fileSize <= 0)
    {
        ERROR_RECOVERABLE("Error: File is empty or invalid size.")
        fclose(file);
        return false;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        ERROR_RECOVERABLE("Error: Failed to seek back to file start.")
        fclose(file);
        return false;
    }

    out_buffer.resize(fileSize);

    size_t const bytesRead = fread(out_buffer.data(), 1, fileSize, file);

    if (bytesRead != static_cast<size_t>(fileSize))
    {
        ERROR_RECOVERABLE("Error: Failed to read the entire file.")
        fclose(file);
        return false;
    }

    int const closeErr = fclose(file);

    if (closeErr != 0)
    {
        ERROR_RECOVERABLE("Error: Failed to close file.")
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool FileReadToString(String& out_string, String const& fileName)
{
    std::vector<uint8_t> buffer;

    if (!FileReadToBuffer(buffer, fileName))
    {
        return false;
    }

    buffer.push_back('\0');

    out_string.assign(reinterpret_cast<char*>(buffer.data()));

    return true;
}

//----------------------------------------------------------------------------------------------------
bool FileWriteFromBuffer(std::vector<uint8_t> const& buffer, String const& fileName)
{
    return FileWriteBinary(fileName, buffer.data(), buffer.size());
}

//----------------------------------------------------------------------------------------------------
bool FileWriteBinary(String const& fileName, void const* data, size_t dataSize)
{
    FILE*   file = nullptr;
    errno_t err  = fopen_s(&file, fileName.c_str(), "wb");
    
    if (err != 0 || file == nullptr)
    {
        ERROR_RECOVERABLE("Error: Failed to open file for writing.")
        return false;
    }

    size_t written = fwrite(data, 1, dataSize, file);
    fclose(file);

    if (written != dataSize)
    {
        ERROR_RECOVERABLE("Error: Failed to write complete data to file.")
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------------------------------
bool EnsureDirectoryExists(String const& directoryPath)
{
    try
    {
        std::filesystem::create_directories(directoryPath);
        return true;
    }
    catch (std::filesystem::filesystem_error const&)
    {
        ERROR_RECOVERABLE("Error: Failed to create directory.")
        return false;
    }
}
