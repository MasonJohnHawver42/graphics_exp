#include "core.h"

#include <shader.pb.h>

#include <SDL2/SDL.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cm.h>

#include <stdexcept>

namespace
{
    GLuint compileShader(GLenum type, const char *source);
}

namespace gl
{
    Core::Core(const CoreCreateInfo &params) : m_window(nullptr), m_glContext(nullptr), running(false), fullscreen(false), dt(0.0f) 
    {
        m_title = params.title;

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) 
        {
            std::string error_msg = "SDL initialization failed: " + std::string(SDL_GetError());
            ASSERT(false, error_msg);
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        m_window = SDL_CreateWindow(params.title,
                                    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    params.width, params.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        if (!m_window)
        {
            SDL_Quit();
            std::string error_msg = "Window creation failed: " + std::string(SDL_GetError());
            ASSERT(false, error_msg.c_str());
        }

        m_glContext = SDL_GL_CreateContext(m_window);

        if (!m_glContext)
        {
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            std::string error_msg = "SDL OpenGL context creation failed: " + std::string(SDL_GetError());
            ASSERT(false, error_msg.c_str());
        }

        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
        {
            SDL_GL_DeleteContext(m_glContext);
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            std::string error_msg = "GLAD initialization failed.";
            ASSERT(false, error_msg.c_str());        }

        if (GLVersion.major < 4 || (GLVersion.major == 4 && GLVersion.minor < 3))
        {
            SDL_GL_DeleteContext(m_glContext);
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            std::string error_msg = "OpenGL 4.3 or higher is required.";
            ASSERT(false, error_msg.c_str());
        }

        LOG_INFO("gl::Core::create OpenGL %d.%d initialized.", GLVersion.major, GLVersion.minor);

        if (SDL_GL_SetSwapInterval(0) < 0)
        {
            std::string error_msg = "Failed to disable V-Sync: " + std::string(SDL_GetError());
            ASSERT(false, error_msg.c_str());     
        }
    }

    Core::~Core()
    {
        SDL_GL_DeleteContext(m_glContext);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        LOG_INFO("gl::Core::destroy SDL and OpenGL context destroyed.");
    }

    void Core::begin() noexcept
    {
        lastTime = (float)SDL_GetTicks() / 1000.0f;
        accumTime = 0.0f;
        accumFrames = 0;
        running = true;
        fullscreen = false;
        dt = 0.0f;

        int width, height;

        SDL_GetWindowSize(m_window, &width, &height);
        glViewport(0, 0, width, height);
    }

    void Core::startFrame() noexcept
    {
        float curTime = (float)SDL_GetTicks() / 1000.0f;
        dt = curTime - lastTime;
        lastTime = curTime;

        accumTime += dt;
        accumFrames++;

        if (accumTime >= 1.0f)
        {
            float avgDt = accumTime / accumFrames;

            char windowName[256];
            snprintf(windowName, sizeof(windowName), "Graphics Exp : %s [FPS: %.0f (%.2fms)]", m_title, 1.0f / avgDt, avgDt * 1000.0f);
            SDL_SetWindowTitle(m_window, windowName);

            accumTime -= 1.0f;
            accumFrames = 0;

            if (1.0f / avgDt < 15)
            {
                running = false;
            }
        }
    }

    void Core::update(const SDL_Event &event) noexcept
    {
        if (event.type == SDL_QUIT)
        {
            running = false;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == 'f')
        {
            fullscreen = !fullscreen;
            u32 windowFlags = SDL_GetWindowFlags(m_window);
            SDL_SetWindowFullscreen(m_window, fullscreen ? windowFlags | SDL_WINDOW_FULLSCREEN_DESKTOP : windowFlags & (~SDL_WINDOW_FULLSCREEN_DESKTOP));
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            SDL_GetWindowSize(m_window, &width, &height);
            glViewport(0, 0, width, height); // Update OpenGL viewport
            // LOG_INFO("fullscreen: %d %d %d", fullscreen, width, height);
        }
    }

    void Core::endFrame() noexcept
    {
        SDL_GetWindowSize(m_window, &width, &height);
        SDL_GL_SwapWindow(m_window);
    }

    // ShaderProgram -------------------------------------------------------------------

    ShaderProgramCreateInfo::ShaderProgramCreateInfo(const void* data, std::size_t size, std::pmr::memory_resource* mr) :
        vertex_source(mr), fragment_source(mr)
    {
        gl::ShaderFile prog;
        prog.ParseFromArray(data, static_cast<int>(size));

        bool vertex_source_found = false;
        bool fragment_source_found = false;

        for (const auto& src : prog.shaders())
        {
            gl::ShaderStage stage = src.stage();
            const std::string& code = src.source();

            switch (stage) {
                case gl::SHADER_STAGE_VERTEX:
                    vertex_source = code;
                    vertex_source_found = true;
                    break;
                case gl::SHADER_STAGE_FRAGMENT:
                    fragment_source = code;
                    fragment_source_found = true;
                    break;
                case gl::SHADER_STAGE_GEOMETRY:
                case gl::SHADER_STAGE_COMPUTE:
                case gl::SHADER_STAGE_TESS_CONTROL:
                case gl::SHADER_STAGE_TESS_EVAL:
                    break;
                default:
                    break;
            }
        }

        if (!vertex_source_found) 
        {
            ASSERT(false, "ShaderProgramCreateInfo vertex source not found in binary format.");
        }
        if (!fragment_source_found) 
        {
            ASSERT(false, "ShaderProgramCreateInfo vertex source not found in binary format.");
        }
    }
    
    ShaderProgramCreateInfo::ShaderProgramCreateInfo(const cm::MappedFile &prog_file, std::pmr::memory_resource* mr) : ShaderProgramCreateInfo(prog_file.data(), prog_file.mappedSize(), mr) {}

    ShaderProgram::ShaderProgram(const ShaderProgramCreateInfo &params, std::pmr::memory_resource* mr) : program(0), uniforms(mr)
    {
        GLuint vertex_shader = compileShader(GL_VERTEX_SHADER, params.vertex_source.c_str());
        GLuint fragment_shader = compileShader(GL_FRAGMENT_SHADER, params.fragment_source.c_str());
        
        program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        // Check for linking errors
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            std::string error_msg = std::string("Shader linking failed: ") + infoLog;
            ASSERT(false, error_msg);
        }

        GLint num_uniforms = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &num_uniforms);

        GLint max_name_length = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_name_length);

        std::vector<GLchar> name_buf(max_name_length);

        for (GLint i = 0; i < num_uniforms; ++i) {
            GLsizei length = 0;
            GLint size = 0;
            GLenum type = 0;

            glGetActiveUniform(program, i, max_name_length, &length, &size, &type, name_buf.data());

            std::string name(name_buf.data(), length);
            GLint location = glGetUniformLocation(program, name.c_str());

            // Don't store uniforms that are optimized out (location == -1)
            if (location != -1) {
                uniforms.emplace(std::move(name), Uniform{type, location});
            }
        }

        LOG_DEBUG("gl::ShaderProgram::create (%u)", program);
    }

    ShaderProgram::~ShaderProgram()
    {
        glDeleteProgram(program);
        LOG_DEBUG("gl::ShaderProgram::destroy (%u)", program);
    }

    void ShaderProgram::use() noexcept 
    {
        glUseProgram(program);
    }

    GLint ShaderProgram::getUniformLocation(const char *name, GLenum expectedType)
    {
        auto it = uniforms.find(name);
        if (it == uniforms.end()) {
            std::string error_msg = std::string("Uniform not found: ") + name;
            ASSERT(false, error_msg);
        }
        if (it->second.type != expectedType) {
            std::string error_msg = std::string("Uniform type mismatch for: ") + name +
                                    " (expected GL type " + std::to_string(expectedType) +
                                    ", got " + std::to_string(it->second.type) + ")";
            ASSERT(false, error_msg);
        }
        return it->second.location;
    }

    std::ostream &operator<<(std::ostream &os, const ShaderProgram &shader) noexcept
    {
        std::ostringstream infoLog;

        // Query Active Attributes (Inputs)
        GLint numAttributes = 0;
        glGetProgramiv(shader.program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
        infoLog << "Inputs (" << numAttributes << "):\n";

        for (GLint i = 0; i < numAttributes; ++i)
        {
            char name[256];
            GLenum type;
            GLint size;
            glGetActiveAttrib(shader.program, i, sizeof(name), nullptr, &size, &type, name);
            GLint location = glGetAttribLocation(shader.program, name);

            infoLog << "  Location " << location << ": " << name << " (type 0x"
                    << std::hex << type << std::dec << ")\n";
        }

        // Query Active Uniforms
        GLint numUniforms = 0;
        glGetProgramiv(shader.program, GL_ACTIVE_UNIFORMS, &numUniforms);
        infoLog << "\nUniforms (" << numUniforms << "):\n";

        for (GLint i = 0; i < numUniforms; ++i)
        {
            char name[256];
            GLenum type;
            GLint size;
            glGetActiveUniform(shader.program, i, sizeof(name), nullptr, &size, &type, name);
            GLint location = glGetUniformLocation(shader.program, name);

            infoLog << "  Location " << location << ": " << name << " (type 0x"
                    << std::hex << type << std::dec << ", size " << size << ")\n";
        }

        // Query Active Buffer Bindings (SSBOs)
        GLint numBuffers = 0;
        glGetProgramInterfaceiv(shader.program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numBuffers);
        infoLog << "\nShader Storage Buffers (SSBOs) (" << numBuffers << "):\n";

        for (GLint i = 0; i < numBuffers; ++i)
        {
            char name[256];
            glGetProgramResourceName(shader.program, GL_SHADER_STORAGE_BLOCK, i, sizeof(name), nullptr, name);

            GLenum properties[] = {GL_TYPE, GL_BUFFER_BINDING};
            GLint values[2];
            glGetProgramResourceiv(shader.program, GL_SHADER_STORAGE_BLOCK, i, 2, properties, 2, nullptr, values);

            infoLog << "  Binding " << values[1] << ": " << name << " (type 0x"
                    << std::hex << values[0] << std::dec << ")\n";
        }

        // Query Active Outputs
        GLint numOutputs = 0;
        glGetProgramInterfaceiv(shader.program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &numOutputs);
        infoLog << "\nOutputs (" << numOutputs << "):\n";

        for (GLint i = 0; i < numOutputs; ++i)
        {
            char name[256];
            glGetProgramResourceName(shader.program, GL_PROGRAM_OUTPUT, i, sizeof(name), nullptr, name);

            GLenum properties[] = {GL_TYPE, GL_LOCATION};
            GLint values[2];
            glGetProgramResourceiv(shader.program, GL_PROGRAM_OUTPUT, i, 2, properties, 2, nullptr, values);

            infoLog << "  Location " << values[1] << ": " << name << " (type 0x"
                    << std::hex << values[0] << std::dec << ")\n";
        }

        // Print the final info
        os << infoLog.str();
        return os;
    }

    glm::mat4 Camera::getViewMatrix() const {
        glm::mat4 rot = glm::mat4_cast(glm::conjugate(orientation));
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), -position);
        return rot * trans;
    }

    glm::mat4 Camera::getProjectionMatrix() const {
        return glm::perspective(fov, aspect, nearClip, farClip);
    }

    // Image -------------------------------------------------------------------

    Image::Image(const std::filesystem::path &path, ImageFormat forceFormat)
    {
        std::string error_msg = "Image::Image Image file does not exist: " + path.string();
        ASSERT(std::filesystem::exists(path), error_msg);
    
        int w = 0, h = 0, c = 0;
    
        stbi_set_flip_vertically_on_load(true); // Flip images vertically by default
    
        int desiredChannels = 0;
        switch (forceFormat)
        {
            case ImageFormat::R: desiredChannels = 1; break;
            case ImageFormat::RG: desiredChannels = 2; break;
            case ImageFormat::RGB: desiredChannels = 3; break;
            case ImageFormat::RGBA: desiredChannels = 4; break;
            default: desiredChannels = 0; break; // Let stb decide
        }
    
        // HDR load check
        if (stbi_is_hdr(path.string().c_str())) {
            isHDR = true;
            floatData = stbi_loadf(path.string().c_str(), &w, &h, &c, desiredChannels);
            if (!floatData) {
                error_msg = "Image::Image Failed to load HDR image: " + path.string() + " | " + stbi_failure_reason();
                ASSERT(false, error_msg);
            }
            data = nullptr;
            channels = desiredChannels ? desiredChannels : c;
        } else {
            data = stbi_load(path.string().c_str(), &w, &h, &c, desiredChannels);
            if (!data) {
                error_msg = "Image::Image Failed to load image: " + path.string() + " | " + stbi_failure_reason();
                ASSERT(false, error_msg);
            }
            floatData = nullptr;
            channels = desiredChannels ? desiredChannels : c;
        }
    
        width = static_cast<unsigned int>(w);
        height = static_cast<unsigned int>(h);
    
        LOG_DEBUG("gl::Image::create \"%s\" (%dx%d, %d channels, %s)", path.string().c_str(), width, height, channels, isHDR ? "HDR" : "LDR");
    }
    
    Image::~Image()
    {
        if (data) {
            stbi_image_free(data);
            data = nullptr;
        }
        if (floatData) {
            stbi_image_free(floatData);
            floatData = nullptr;
        }
    
        LOG_DEBUG("gl::Image::destroy (%dx%d, %d channels)", width, height, channels);
    }

    Texture::Texture(const Image &image, const TextureCreateInfo &params) : width(image.width), height(image.height), channels(image.channels), id(0)
    {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
    
        // Use format from params or infer based on image channels if not specified
        GLenum format = params.format;
        GLenum internalFormat = params.internalFormat;
        GLenum type = params.type;
    
        // If params left at defaults, infer from image
        if (format == 0) 
        {
            switch (channels) 
            {
                case 0:
                case 4:
                format = GL_RGBA;
                internalFormat = GL_RGBA8;
                break;
                
                case 1:
                format = GL_RED;
                internalFormat = GL_R8;
                break;
                
                case 2:
                format = GL_RG;
                internalFormat = GL_RG8;
                break;

                case 3:
                    format = GL_RGB;
                    internalFormat = GL_RGB8;
                    break;
                    
                    default:
                    // Handle error: unexpected channel count
                    STREAM_ERROR << "Unsupported number of channels: " << channels;
                    format = GL_RGBA;
                    internalFormat = GL_RGBA8;
                    break;
            }
            type = GL_UNSIGNED_BYTE;
        }

        // LOG_DEBUG("texture channels: %d, format: %s, internalFormat: %s, type: %s", channels, fGetEnumName(format).c_str(), fGetEnumName(internalFormat).c_str(), fGetEnumName(type).c_str());
    
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, type, image.data);
    
        if (params.generateMipmaps) 
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.magFilter);
    
        glBindTexture(GL_TEXTURE_2D, 0);
    
        LOG_DEBUG("gl::Texture::create (%dx%d, %d channels) (%u)", width, height, channels, id);
    }

    Texture::~Texture()
    {
        GLuint temp_id = id;

        if (id != 0) {
            glDeleteTextures(1, &id);
            id = 0;
        }

        LOG_DEBUG("gl::Texture::destroy (%dx%d, %d channels) (%u)", width, height, channels, temp_id);
    }
}

namespace
{
    GLuint compileShader(GLenum type, const char *source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::string error_msg = std::string("Shader compilation failed: ") + infoLog;
            ASSERT(false, error_msg);
        }
        return shader;
    }
}