#include "sys.h"
#include "log.h"

#ifdef PLATFORM_LINUX

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace cm
{
    MappedFile::MappedFile(const std::filesystem::path &file_path, size_t map_range, u8 cache_hint) noexcept
    {
        m_impl.fd = open(file_path.c_str(), O_RDONLY);
        if (m_impl.fd == -1)
        {
            m_impl.mappedData = nullptr;
            m_impl.fileSize = 0;
            m_impl.mappedSize = 0;
            return;            
        }

        m_impl.fileSize = lseek(m_impl.fd, 0, SEEK_END);
        m_impl.mappedSize = (map_range == WholeFile) ? m_impl.fileSize : map_range;

        m_impl.mappedData = mmap(nullptr, m_impl.mappedSize, PROT_READ, MAP_PRIVATE, m_impl.fd, 0);
        if (m_impl.mappedData == MAP_FAILED)
        {
            close(m_impl.fd);
            m_impl.fd = -1;
            m_impl.mappedData = nullptr;
            m_impl.fileSize = 0;
            m_impl.mappedSize = 0;
            return;
        }

        STREAM_DEBUG << "cm::MappedFile::create " << file_path << " (" << m_impl.fd << ")";
    }

    MappedFile::~MappedFile()
    {
        destroy();
    }

    MappedFile::MappedFile(MappedFile &&other) noexcept
        : m_impl{other.m_impl}
    {
        other.m_impl.fd = -1;
        other.m_impl.mappedData = nullptr;
        other.m_impl.fileSize = 0;
        other.m_impl.mappedSize = 0;
    }

    MappedFile &MappedFile::operator=(MappedFile &&other) noexcept
    {
        if (this != &other)
        {
            destroy();
            m_impl = other.m_impl;
            other.m_impl.fd = -1;
            other.m_impl.mappedData = nullptr;
            other.m_impl.fileSize = 0;
            other.m_impl.mappedSize = 0;
        }
        return *this;
    }

    void MappedFile::destroy()
    {
        if (m_impl.mappedData)
        {
            STREAM_DEBUG << "cm::MappedFile::unmapped (" << m_impl.fd << ")";
            munmap(m_impl.mappedData, m_impl.mappedSize);
            m_impl.mappedData = nullptr;
        }
        if (m_impl.fd != -1)
        {
            STREAM_DEBUG << "cm::MappedFile::closed (" << m_impl.fd << ")";
            close(m_impl.fd);
            m_impl.fd = -1;
        }
        
        m_impl.fd = -1;
        m_impl.mappedData = nullptr;
        m_impl.fileSize = 0;
        m_impl.mappedSize = 0;
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

    void MappedFile::remap(uint64_t offset, size_t mappedBytes) noexcept
    {
        if (m_impl.fd == -1)
        {
            destroy();
            return;
        }

        void *newMapping = mmap(nullptr, mappedBytes, PROT_READ, MAP_PRIVATE, m_impl.fd, offset);
        if (newMapping == MAP_FAILED)
        {
            
            destroy();
            return;
        }

        if (m_impl.mappedData)
        {
            munmap(m_impl.mappedData, m_impl.mappedSize);
        }

        m_impl.mappedData = newMapping;
        m_impl.mappedSize = mappedBytes;
    }

}

#endif