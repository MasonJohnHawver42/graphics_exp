#ifndef GL_CORE_H
#define GL_CORE_H

#include <SDL2/SDL.h>
#include <glad/glad.h>

#include <cm.h>

#include <unordered_map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace gl
{

    struct CoreCreateInfo
    {
        const char *title;
        u32 width, height;
    };

    class Core
    {
    public:
        Core(const CoreCreateInfo &params);
        ~Core();

        // No copying
        Core(const Core &other) = delete;
        Core &operator=(const Core &) = delete;

        // No moving
        Core(Core &&other) = delete;
        Core &operator=(Core &&other) = delete;

        void begin() noexcept;
        void startFrame() noexcept;
        void update(const SDL_Event &event) noexcept;
        void endFrame() noexcept;

    public:
        SDL_Window *m_window;
        bool running;
        bool fullscreen;
        float dt;

    private:
        SDL_GLContext m_glContext;
        const char *m_title;

        float lastTime;
        float accumTime;
        int accumFrames;

        int width, height;
    };

    struct ShaderProgramCreateInfo
    {
        ShaderProgramCreateInfo(const void* data, std::size_t size, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        ShaderProgramCreateInfo(const cm::MappedFile &prog_file, std::pmr::memory_resource* mr = std::pmr::get_default_resource());
        // ShaderProgramCreateInfo(const char *vertex_source, const char *fragment_source) : vertex_source(vertex_source), fragment_source(fragment_source) {}

        // const char *vertex_source;
        // const char *fragment_source;
        std::pmr::string vertex_source;
        std::pmr::string fragment_source;
    };

    class ShaderProgram
    {
    public:
        ShaderProgram(const ShaderProgramCreateInfo &params, std::pmr::memory_resource* mr = std::pmr::get_default_resource());

        ~ShaderProgram();

        void use() noexcept;

        GLint getUniformLocation(const char *name, GLenum expectedType);
        
        friend std::ostream &operator<<(std::ostream &os, const ShaderProgram &shader) noexcept;

    private:
        void checkUniformType(const char *name, GLenum expectedType);

        struct Uniform 
        {
            GLenum type;
            GLint location;
        };

        std::pmr::unordered_map<std::string, Uniform> uniforms;
        GLuint program;
    };

    struct Camera 
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
        float fov = glm::radians(60.0f);
        float aspect = 16.0f / 9.0f;
        float nearClip = 0.1f;
        float farClip = 1000.0f;

        glm::mat4 getViewMatrix() const;
        glm::mat4 getProjectionMatrix() const;
    };
}

#endif