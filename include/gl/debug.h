#ifndef _GL_DEBUG_H
#define _GL_DEBUG_H

#include "core.h"

#include <memory_resource>
#include <unordered_map>
#include <string>

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <cm.h>

#include <iomanip>
#include <iostream>

namespace gl::debug
{

    // -----------------------------------------------------------------

    struct DrawCommand 
    {
        glm::mat4 model;
        glm::vec3 color;
        unsigned int id;
    };

    struct FrameCommand 
    {
        glm::mat4 view;
    };

    class IRenderer 
    {
    public:
        virtual ~IRenderer();
        virtual void begin(const FrameCommand& cmd) = 0;
        virtual void submit(const DrawCommand& cmd) = 0;
        virtual void end() = 0;

        virtual unsigned int model_id(std::string&& query) = 0;
    };

    // -----------------------------------------------------------------

    struct Vertex 
    {
        float position[3];
        float normal[3];
    };

    struct Model 
    {
        unsigned int vert_offset, ebo_offset, vert_length, ebo_length;
    };

    struct ModelCacheCreateInfo 
    {
        ModelCacheCreateInfo(const void* data, std::size_t size, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        ModelCacheCreateInfo(const cm::MappedFile& cache_file, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        // ModelCacheCreateInfo(cm::ZCompressedData& zc_data, std::pmr::memory_resource* resource = std::pmr::get_default_resource());

        std::pmr::unordered_map<std::string, unsigned int> models;
        std::pmr::vector<Model> primitives;
        std::pmr::vector<Vertex> vertices;
        std::pmr::vector<unsigned int> indices;
    };

    struct ModelCache
    {
        ModelCache(const ModelCacheCreateInfo& params, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        ~ModelCache();
    
        std::pmr::unordered_map<std::string, unsigned int> models;
        std::pmr::vector<Model> primitives;
        GLuint vao, vbo, ebo;
    };

    // -----------------------------------------------------------------

    class NaiveRenderer : public IRenderer {
    public:
        NaiveRenderer(const ShaderProgramCreateInfo& shader_param, const ModelCacheCreateInfo& cache_param, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        ~NaiveRenderer() override = default;
    
        void begin(const FrameCommand& cmd) override;
        void submit(const DrawCommand& cmd) override;
        void end() override;

        unsigned int model_id(std::string&& query) override;
    
    private:
        ShaderProgram program;
        ModelCache cache;
    
        FrameCommand current_frame;
        std::pmr::vector<DrawCommand> draw_queue;
    
        // Uniform locations
        GLint u_model, u_view, u_color;
    };

    // struct DebugPrimitiveInstance
    // {
    //     glm::mat4 model;
    //     glm::vec3 color;
    // };

    // class DebugPrimitiveDrawCache
    // {
    //     public:
    //     DebugPrimitiveDrawCache();
    //     ~DebugPrimitiveDrawCache();

    //     void reset();
    //     void add(unsigned int model_id, const DebugPrimitiveInstance& instance);

    //     private:
    //         std::pmr::unordered_map<unsigned int, std::pmr::vector<DebugPrimitiveInstance>> instances;
    // };

    // class DebugPrimitivesPass 
    // {
    //     public:
    //         DebugPrimitivesPass(ModelCacheCreateInfo& params);
    //         ~DebugPrimitivesPass();

    //         void draw(const DebugPrimitiveDrawParams& command);

    //     private:
    //         ModelCache model_data;
    //         ShaderProgram shader;

    //         GLuint primitive_model_instance_ssbo;
    //         GLuint draw_arrays_indirect_commands;
    // };

}

std::ostream& operator<<(std::ostream& os, const gl::debug::ModelCacheCreateInfo& info);

#endif // _DEBUG_H