#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <memory_resource>
#include <filesystem>
#include <vector>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

#ifdef PLATFORM_LINUX
// No includes needed
#endif

#include <zlib.h>

#include "types.h"
#include "log.h"

namespace cm
{

    class MappedFile
    {
    public:
        enum CacheHint : u8
        {
            Normal,         ///< good overall performance
            SequentialScan, ///< read file only once with few seeks
            RandomAccess    ///< jump around
        };

        /// how much should be mappend
        enum MapRange : size_t
        {
            WholeFile = 0 ///< everything ... be careful when file is larger than memory
        };

        explicit MappedFile(const std::filesystem::path &file_path, size_t map_range = WholeFile, u8 cache_hint = Normal) noexcept;
        ~MappedFile();

        // No copying
        MappedFile(const MappedFile &) = delete;
        MappedFile &operator=(const MappedFile &) = delete;

        // Move semantics
        MappedFile(MappedFile &&other) noexcept;
        MappedFile &operator=(MappedFile &&other) noexcept;

        void *data() noexcept;
        const void *data() const noexcept;

        size_t fileSize() const noexcept;
        size_t mappedSize() const noexcept;

        void remap(uint64_t offset, size_t mappedBytes) noexcept;

    private:
        void destroy();

        struct Impl
        {
#ifdef PLATFORM_LINUX
            int fd;
            void *mappedData;
            size_t fileSize;
            size_t mappedSize;
#elif PLATFORM_WINDOWS
            HANDLE fileHandle;
            HANDLE mappingHandle;
            void *mappedData;
            size_t fileSize;
            size_t mappedSize;
#endif
        } m_impl;
    };

    class ZCompressedData
    {
    public:
        using allocator_type = std::pmr::polymorphic_allocator<>;

        explicit ZCompressedData(const void *data, size_t data_size) noexcept;
        explicit ZCompressedData(const MappedFile &mapped_file) noexcept;

        explicit ZCompressedData(const void *data, size_t data_size, const allocator_type &alloc) noexcept;
        explicit ZCompressedData(const MappedFile &mapped_file, const allocator_type &alloc) noexcept;

        ZCompressedData &operator=(const ZCompressedData &) = default;
        ZCompressedData &operator=(ZCompressedData &&other) = default;

        ZCompressedData(const ZCompressedData &) = default;
        ZCompressedData(ZCompressedData &&other) = default;

        ~ZCompressedData() = default;

        void *data() noexcept;
        const void *data() const noexcept;
        size_t size() const noexcept;

    private:
        void destroy();
        b8 uncompress(const Bytef *compressed_data, std::size_t compressed_size);

        std::pmr::vector<unsigned char> m_data;
    };

} // namespace cm

#endif // SYSTEM_H