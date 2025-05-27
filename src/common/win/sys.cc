#include "sys.h"

#ifdef PLATFORM_WINDOWS

#include <Windows.h>
#include <stdexcept>
#include <filesystem>

namespace cm
{

    MappedFile::MappedFile(const std::filesystem::path &file_path, size_t map_range, u8 cache_hint)
    {
        m_impl.fileHandle = CreateFile(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_impl.fileHandle == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error("Failed to open file: " + file_path.string());
        }

        m_impl.fileSize = GetFileSize(m_impl.fileHandle, nullptr);
        if (m_impl.fileSize == INVALID_FILE_SIZE)
        {
            CloseHandle(m_impl.fileHandle);
            throw std::runtime_error("Failed to get file size: " + file_path.string());
        }

        m_impl.mappedSize = (map_range == WholeFile) ? m_impl.fileSize : map_range;
        m_impl.mappingHandle = CreateFileMapping(m_impl.fileHandle, NULL, PAGE_READONLY, 0, static_cast<DWORD>(m_impl.mappedSize), NULL);
        if (m_impl.mappingHandle == NULL)
        {
            CloseHandle(m_impl.fileHandle);
            throw std::runtime_error("Failed to create file mapping: " + file_path.string());
        }

        m_impl.mappedData = MapViewOfFile(m_impl.mappingHandle, FILE_MAP_READ, 0, 0, m_impl.mappedSize);
        if (m_impl.mappedData == nullptr)
        {
            CloseHandle(m_impl.mappingHandle);
            CloseHandle(m_impl.fileHandle);
            throw std::runtime_error("Failed to map file: " + file_path.string());
        }
    }

    MappedFile::~MappedFile()
    {
        destroy();
    }

    MappedFile::MappedFile(MappedFile &&other) noexcept
        : m_impl(other.m_impl)
    {
        other.m_impl = {nullptr, nullptr, nullptr, 0, 0};
    }

    MappedFile &MappedFile::operator=(MappedFile &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            m_impl = other.m_impl;
            other.m_impl = {nullptr, nullptr, nullptr, 0, 0};
        }
        return *this;
    }

    void MappedFile::destroy()
    {
        if (m_impl.mappedData)
        {
            UnmapViewOfFile(m_impl.mappedData);
            m_impl.mappedData = nullptr;
        }
        if (m_impl.mappingHandle)
        {
            CloseHandle(m_impl.mappingHandle);
            m_impl.mappingHandle = nullptr;
        }
        if (m_impl.fileHandle)
        {
            CloseHandle(m_impl.fileHandle);
            m_impl.fileHandle = nullptr;
        }
    }

    void *MappedFile::data() noexcept
    {
        return m_impl.mappedData;
    }

    const void *MappedFile::data() const noexcept
    {
        return m_impl.mappedData;
    }

    size_t MappedFile::fileSize() const noexcept
    {
        return m_impl.fileSize;
    }

    size_t MappedFile::mappedSize() const noexcept
    {
        return m_impl.mappedSize;
    }

    void MappedFile::remap(uint64_t offset, size_t mappedBytes)
    {
        if (!m_impl.fileHandle)
        {
            throw std::runtime_error("Cannot remap a closed file");
        }

        destroy(); // Unmap the current mapping

        m_impl.mappedSize = mappedBytes;
        m_impl.mappingHandle = CreateFileMapping(m_impl.fileHandle, NULL, PAGE_READONLY, 0, static_cast<DWORD>(m_impl.mappedSize), NULL);
        if (m_impl.mappingHandle == NULL)
        {
            throw std::runtime_error("Failed to create new file mapping");
        }

        m_impl.mappedData = MapViewOfFile(m_impl.mappingHandle, FILE_MAP_READ, static_cast<DWORD>(offset >> 32), static_cast<DWORD>(offset), m_impl.mappedSize);
        if (m_impl.mappedData == nullptr)
        {
            CloseHandle(m_impl.mappingHandle);
            throw std::runtime_error("Failed to map new file region");
        }
    }

} // namespace cm

#endif
