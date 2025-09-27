#include <cm.h>

#include <string>
#include <iostream>

void print_data(void *data, size_t length)
{
    if (!data || length == 0)
    {
        return;
    }

    u8 *data_ptr = (u8 *)data;

    for (int i = 0; i < length; i++)
    {
        u8 byte = data_ptr[i];
        std::cout << (char)(((byte >> 4) < 10 ? '0' + (byte >> 4) : 'A' + (byte >> 4) - 10));
        std::cout << (char)(((byte & 0xF) < 10 ? '0' + (byte & 0xF) : 'A' + (byte & 0xF) - 10));
        std::cout << " ";
        if (i % 4 == 3)
        {
            std::cout << "  ";
        }
        if (i % 16 == 15)
        {
            std::cout << "\n";
        }
    }

    std::cout << std::endl;
}

int main()
{
    LOG_INFO("hello world!");

#ifdef PLATFORM_LINUX

    LOG_DEBUG("platform: linux");

#endif

    std::cout << std::endl;

    try
    {
        {
            cm::MappedFile json_data("images/cubemaps/rogland_clear_night_4k.cubemap.json");

            LOG_INFO("%s :", "images/cubemaps/rogland_clear_night_4k.cubemap.json");
            print_data(json_data.data(), json_data.mappedSize());
        }

        {
            cm::MappedFile data("images/cubemaps/rogland_clear_night_4k.cubemap.bin");

            LOG_INFO("%s :", "images/cubemaps/rogland_clear_night_4k.cubemap.bin");
            print_data(data.data(), data.mappedSize());
        }

        {
            cm::MappedFile comp_data("images/cubemaps/rogland_clear_night_4k.cubemap.bin.z");
            cm::ZCompressedData uncomp_data(comp_data);

            LOG_INFO("%s :", "images/cubemaps/rogland_clear_night_4k.cubemap.bin.z");
            print_data(comp_data.data(), comp_data.mappedSize());

            LOG_INFO("%s :", "uncompressed images/cubemaps/rogland_clear_night_4k.cubemap.bin.z");
            print_data(uncomp_data.data(), uncomp_data.size());
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Exception caught: %s", e.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        LOG_ERROR("Unknown exception caught!");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}