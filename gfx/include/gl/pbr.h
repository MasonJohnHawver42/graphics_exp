#ifndef _PBR_H
#define _PBR_H

#include "core.h"

#include <memory_resource>
#include <unordered_map>
#include <filesystem>
#include <string>

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <cm.h>

namespace gl::pbr 
{
    struct Vertex 
    {
        glm::vec3 position, normal, tangent, bitangent;
        glm::vec2 texCoord;
    };

    struct MeshCreateInfo 
    {
        std::pmr::vector<Vertex> vertices;
        std::pmr::vector<unsigned int> indices;
        std::pmr::string name;
    };

    struct MeshEntry 
    {
        size_t vertexOffset, vertexCount, indexOffset, indexCount;
    };

    struct MeshCache 
    {
        std::pmr::vector<MeshEntry> meshes;

        size_t vertexCap, vertexOffset, indexCap, indexOffset;
        GLuint vao, vbo, ebo;
    };

    //

    enum class TextureType : u8
    {
        ALBEDO = 0,
        MRAO,
        NORMAL,
        EMISSIVE,
        COUNT
    };

    std::ostream& operator<<(std::ostream& os, TextureType type);

    struct TexturePoolCreateInfo 
    {

        TexturePoolCreateInfo(TextureType t, GLsizei width, GLsizei height, GLsizei layers);

        // Filtering
        GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;
        GLint magFilter = GL_LINEAR;

        // Wrapping modes
        GLint wrapS = GL_REPEAT;
        GLint wrapT = GL_REPEAT;

        // Mipmap control
        bool generateMipmaps = true;

        // Internal format override (optional)
        GLint internalFormat = GL_RGBA8;
        GLenum format = GL_RGBA;
        GLenum type = GL_UNSIGNED_BYTE;

        // Dimensions of each layer
        GLsizei width = 0;
        GLsizei height = 0;

        // Number of layers in the array
        GLsizei layerCount = 1;
        
        TextureType pool_type;
    };

    std::ostream& operator<<(std::ostream& os, const TexturePoolCreateInfo& info);

    struct TextureCacheCreateInfo
    {
        // this accepts the fbs data file
        TextureCacheCreateInfo(const void* data, std::size_t size, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        TextureCacheCreateInfo(const cm::MappedFile &prog_file, std::pmr::memory_resource* mr = std::pmr::get_default_resource());

        struct ImageIndex 
        {
            std::filesystem::path path;
            unsigned int pool;
            TextureType type;
        };

        std::pmr::vector<TexturePoolCreateInfo> pools; // only create eough layers to fit all images in the pool, if there are no images asociated with the pool dont add it
        std::pmr::unordered_map<std::pmr::string, ImageIndex> textures; // image path to pool index
        // when loading the image paths resolve them so they are unique to the working directory use std::filesystem
    
        std::pmr::string default_textures[static_cast<std::size_t>(TextureType::COUNT)]; // indexed by TextureType
    };

    std::ostream& operator<<(std::ostream& os, const TextureCacheCreateInfo::ImageIndex& idx);
    std::ostream& operator<<(std::ostream& os, const TextureCacheCreateInfo& cache);

    #define TEXTURE_POOL_MAX_IMAGE_CAP 256

    struct TextureCache 
    {
        struct Pool 
        {
            // cm::FlagArray<TEXTURE_POOL_MAX_IMAGE_CAP> capacity;

            GLuint texture_id;
            GLint texture_unit;
            
            unsigned int image_cap;
            unsigned int next_free; 

            TextureType pool_type;
            unsigned int width, height;
        };
        
        struct Texture 
        {
            std::filesystem::path path;
            unsigned int pool, layer;
            TextureType type; // pool type: 0 - Albedo, 1 - MRAO, 2 - Normal, 3 - Emissive
            b8 loaded;
        };
        
        struct TextureIndex 
        {
            GLint texture_unit;
            unsigned int layer;
        };
        
        struct DefaultTexture
        {
            unsigned int pool, layer;
            TextureType type;
        };

        // create the texture arrays based on the create info
        // then load each default image into its respective pool
        TextureCache(const TextureCacheCreateInfo& info, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        ~TextureCache();

        void bind();
        void unbind();

        // only load paths in the map, assert otherwise
        void load(const Image& img, const char* query);

        // only works if textures bound
        // query is the name of the path (all the textures are in the same folder so the key for fetching them should just be their name ie texture0.png)
        // if the image is unloaded 
        TextureIndex fetch(const char* query);
        
        std::pmr::vector<Pool> texture_pools;
        // the key is just the name of the texture ie texture0.png
        std::pmr::unordered_map<std::pmr::string, Texture> textures;
        DefaultTexture default_textures[static_cast<std::size_t>(TextureType::COUNT)];

    };
}

#endif // _PBR_H