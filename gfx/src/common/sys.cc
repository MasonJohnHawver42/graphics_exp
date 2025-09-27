#include "sys.h"

// todo

#include <zlib.h>

#include <stdexcept>

namespace cm
{

    b8 cm::ZCompressedData::uncompress(const Bytef *compressed_data, std::size_t compressed_size)
    {
        z_stream strm{};
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = compressed_size;
        strm.next_in = const_cast<Bytef *>(compressed_data);

        if (inflateInit(&strm) != Z_OK)
        {
            return false;
        }

        m_data.clear();

        constexpr size_t CHUNK_SIZE = 16384;
        Bytef out[CHUNK_SIZE];

        int ret;
        do
        {
            strm.avail_out = CHUNK_SIZE;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);

            if (ret != Z_OK && ret != Z_STREAM_END)
            {
                inflateEnd(&strm);
                m_data.clear();
                return false;
            }

            size_t have = CHUNK_SIZE - strm.avail_out;
            m_data.insert(m_data.end(), out, out + have);
        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);

        return true;
    }

    ZCompressedData::ZCompressedData(const void *data, size_t data_size) noexcept : ZCompressedData(data, data_size, std::pmr::get_default_resource()) {}
    ZCompressedData::ZCompressedData(const MappedFile &mapped_file) noexcept : ZCompressedData(mapped_file.data(), mapped_file.mappedSize(), std::pmr::get_default_resource()) {}

    ZCompressedData::ZCompressedData(const MappedFile &mapped_file, const allocator_type &alloc) noexcept : ZCompressedData(mapped_file.data(), mapped_file.mappedSize(), alloc) {}

    ZCompressedData::ZCompressedData(const void *data, size_t data_size, const allocator_type &alloc) noexcept : m_data(alloc)
    {
        if (data == nullptr || data_size == 0)
        {
            m_data.clear();
            return;
        }

        b8 result = uncompress(static_cast<const Bytef *>(data), data_size);

        if (!result)
        {
            m_data.clear();
            return;
        }
    }

    // Return pointer to the data
    void *cm::ZCompressedData::data() noexcept
    {
        return m_data.data();
    }

    // Return const pointer to the data
    const void *cm::ZCompressedData::data() const noexcept
    {
        return m_data.data();
    }

    // Return the size of the uncompressed data
    size_t cm::ZCompressedData::size() const noexcept
    {
        return m_data.size();
    }
}