// #include "pbr.h"

// #include "data/texture_pool_generated.h"

// #include <string_view>

// namespace gl::pbr 
// {
//     TexturePoolCreateInfo::TexturePoolCreateInfo(TextureType t, GLsizei w, GLsizei h, GLsizei l) :
//         width(w), height(h), layerCount(l > 1 ? l : 1), pool_type(t)
//     {
//         switch(t) 
//         {
//             case TextureType::ALBEDO:
//                 internalFormat = GL_SRGB8_ALPHA8;
//                 format = GL_RGBA;
//                 type = GL_UNSIGNED_BYTE;
//             break;
//             case TextureType::MRAO:
//                 internalFormat = GL_RGB8;
//                 format = GL_RGB;
//                 type = GL_UNSIGNED_BYTE;
//             break;
//             case TextureType::NORMAL:
//                 internalFormat = GL_RG8; // 2-channel normals
//                 format = GL_RG;
//                 type = GL_UNSIGNED_BYTE;
//             break;
//             case TextureType::EMISSIVE:
//                 internalFormat = GL_SRGB8;
//                 format = GL_RGB;
//                 type = GL_UNSIGNED_BYTE;
//             break;
//         }
//     }

//     TextureCacheCreateInfo::TextureCacheCreateInfo(const cm::MappedFile &prog_file, std::pmr::memory_resource* mr) : TextureCacheCreateInfo(prog_file.data(), prog_file.mappedSize(), mr) {}

//     TextureCacheCreateInfo::TextureCacheCreateInfo(const void* data, std::size_t size, std::pmr::memory_resource* mr) :
//         pools(mr), textures(mr)
//     {
//         auto poolList = gl::pbr::GetTexturePoolList(data);

//         unsigned int pool_index = 0;
//         for (const auto* pool : *poolList->pools()) {
//             if (!pool || !pool->image_paths() || pool->image_paths()->size() == 0)
//                 continue;

//             TexturePoolCreateInfo pool_info(
//                 static_cast<TextureType>(pool->pool_type()), 
//                 pool->width(), 
//                 pool->height(), 
//                 pool->image_paths()->size()
//             );

//             for (const auto* img_path : *pool->image_paths()) 
//             {
//                 if (!img_path) continue;
    
//                 std::filesystem::path abs_path = std::filesystem::absolute(img_path->str());
//                 std::pmr::string key(abs_path.filename().string(), mr);
    
//                 textures.emplace(std::move(key), (TextureCacheCreateInfo::ImageIndex) {abs_path, pool_index, static_cast<TextureType>(pool->pool_type())});
//             }

//             pools.push_back(pool_info);
//             ++pool_index;
//         }

//         for (size_t i = 0; i < poolList->defaults()->size() && i < static_cast<std::size_t>(TextureType::COUNT); ++i) {
//             const auto* def = poolList->defaults()->Get(i);
//             if (def)
//                 default_textures[i] = std::pmr::string(def->str(), mr);
//         }

//             // pool_info.channels = pool->channels();

//     }

//     TextureCache::TextureCache(const TextureCacheCreateInfo& info, std::pmr::memory_resource* mr) :
//         texture_pools(mr), textures(mr)
//     {
//         texture_pools.reserve(info.pools.size());

//         for (const TexturePoolCreateInfo& pool_info : info.pools) 
//         {
//             GLuint textureID;
//             glGenTextures(1, &textureID);
//             glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

//             // Set texture parameters
//             glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, pool_info.minFilter);
//             glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, pool_info.magFilter);
//             glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, pool_info.wrapS);
//             glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, pool_info.wrapT);

//             // Allocate texture storage (no data upload yet)
//             glTexImage3D(
//                 GL_TEXTURE_2D_ARRAY,
//                 0,                    // mipmap level 0
//                 pool_info.internalFormat, // internal format
//                 pool_info.width,
//                 pool_info.height,
//                 pool_info.layerCount,     // depth = number of layers
//                 0,                   // border
//                 pool_info.format,         // format of data (if uploading later)
//                 pool_info.type,           // data type (e.g., GL_UNSIGNED_BYTE)
//                 nullptr              // no initial data
//             );

//             // Generate mipmaps if requested
//             // if (pool_info.generateMipmaps) {
//             //     glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
//             // }

//             glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            
//             Pool pool = {.texture_id = textureID, .texture_unit=-1, .image_cap = pool_info.layerCount, .next_free = 0, .pool_type = pool_info.pool_type, .width = pool_info.width, .height = pool_info.height};
//             texture_pools.emplace_back(pool);
//         }

//         for (const auto& [key, value] : info.textures) 
//         {
//             if (value.type != TextureType::ALBEDO) { continue; }

//             Image img(value.path, ImageFormat::RGBA);

//             Pool& current = texture_pools[value.pool];

//             ASSERT(value.type == current.pool_type, "todo");
//             ASSERT(img.width == current.width, "todo");
//             ASSERT(img.height == current.height, "todo");

//             GLenum format = GL_RGBA;
//             GLenum internalFormat = GL_RGBA8;

//             glBindTexture(GL_TEXTURE_2D_ARRAY, current.texture_id);

//             glTexSubImage3D(
//                 GL_TEXTURE_2D_ARRAY,
//                 0,                      // mip level
//                 0, 0, current.next_free,      // x, y, z offset
//                 img.width, img.height, 1,       // width, height, depth=1
//                 format,
//                 GL_UNSIGNED_BYTE,
//                 img.data
//             );

//             std::pmr::string name(key, mr);
//             Texture texture = {.path = value.path, .pool = value.pool, .layer = current.next_free++, .type = value.type, .loaded = true};

//             textures.emplace(std::move(name), std::move(texture));
//         }

//         for (const Pool& pool : texture_pools) 
//         {
//             glBindTexture(GL_TEXTURE_2D_ARRAY, pool.texture_id);

//             glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

//             glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
//         }

//         unsigned int i = 0;
//         for (const std::pmr::string& default_key : info.default_textures) 
//         {
//             const Texture& texture = textures[default_key];
//             default_textures[i++] = (DefaultTexture) {.pool = texture.pool, .layer = texture.layer, .type = texture.type};
//         }
//     }

//     TextureCache::~TextureCache() 
//     {
//         for (const Pool& pool : texture_pools) 
//         {
//             glDeleteTextures(1, &pool.texture_id);
//         }
//     }

//     void TextureCache::bind() 
//     {
//         GLint max_units = 0;
//         glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
//         ASSERT(texture_pools.size() <= static_cast<size_t>(max_units), "Too many texture pools for available texture units");

//         for (size_t i = 0; i < texture_pools.size(); ++i)
//         {
//             const Pool& pool = texture_pools[i];
//             ASSERT(glIsTexture(pool.texture_id), "Invalid texture ID in pool");

//             glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(i));
//             glBindTexture(GL_TEXTURE_2D_ARRAY, pool.texture_id);
//         }

//         // Optional: reset active texture unit to 0
//         glActiveTexture(GL_TEXTURE0);
//     }

//     std::ostream& operator<<(std::ostream& os, TextureType type) {
//         switch (type) {
//             case TextureType::ALBEDO:   return os << "ALBEDO";
//             case TextureType::MRAO:     return os << "MRAO";
//             case TextureType::NORMAL:   return os << "NORMAL";
//             case TextureType::EMISSIVE: return os << "EMISSIVE";
//             default:                    return os << "UNKNOWN";
//         }
//     }

//     std::ostream& operator<<(std::ostream& os, const TexturePoolCreateInfo& info) {
//         os << "TexturePoolCreateInfo {\n";
//         os << "  pool_type: " << info.pool_type << '\n';
//         os << "  dimensions: " << info.width << "x" << info.height << '\n';
//         os << "  layers: " << info.layerCount << '\n';
//         os << "  minFilter: " << info.minFilter << ", magFilter: " << info.magFilter << '\n';
//         os << "  wrapS: " << info.wrapS << ", wrapT: " << info.wrapT << '\n';
//         os << "  generateMipmaps: " << std::boolalpha << info.generateMipmaps << '\n';
//         os << "  internalFormat: " << info.internalFormat
//         << ", format: " << info.format
//         << ", type: " << info.type << '\n';
//         os << "}";
//         return os;
//     }

//     std::ostream& operator<<(std::ostream& os, const TextureCacheCreateInfo::ImageIndex& idx) {
//         os << "{ path: " << idx.path
//         << ", pool: " << idx.pool
//         << ", type: " << idx.type << " }";
//         return os;
//     }

//     std::ostream& operator<<(std::ostream& os, const TextureCacheCreateInfo& cache) {
//         os << "TextureCacheCreateInfo {\n";

//         os << "  pools:\n";
//         for (const auto& pool : cache.pools) {
//             os << "    " << pool << '\n';
//         }

//         os << "  textures:\n";
//         for (const auto& [key, value] : cache.textures) {
//             os << "    \"" << key << "\": " << value << '\n';
//         }

//         os << "  default_textures:\n";
//         for (std::size_t i = 0; i < static_cast<std::size_t>(TextureType::COUNT); ++i) {
//             os << "    [" << static_cast<TextureType>(i) << "] = \"" << cache.default_textures[i] << "\"\n";
//         }

//         os << "}";
//         return os;
//     }
// };


