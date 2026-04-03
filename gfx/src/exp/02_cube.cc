#include <cm.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <gl.h>

#include "game.h"

namespace game 
{
    class App
    {
    public:
        App(gl::Core& core);
        ~App();
    
        void on_event(const SDL_Event& event);
        void on_update(float dt);
        void on_render(void);
    
    private:
        //Core
        gl::Core& m_core;
    
        // Renderers
        cm::unique_ptr<gl::debug::primitives::IRenderer> m_debugPass;
        cm::unique_ptr<gl::debug::quads::IRenderer>      m_quadPass;
    
        // Game Info
        cm::unique_ptr<gl::Camera>                  m_camera;
        cm::unique_ptr<game::FlyByCameraController> m_flybyController;
        cm::unique_ptr<game::CTACameraController>   m_ctaController;
        
        cm::unique_ptr<gl::Texture>                 m_texture;
    
        // Data Buffers
        std::byte main_buffer[1024 * 10];
        std::pmr::monotonic_buffer_resource main_resource{main_buffer, 1024 * 10};

        std::byte temp_buffer[1024 * 2];
        std::pmr::monotonic_buffer_resource temp_resource{temp_buffer, 1024 * 2};

        // Misc
        float time = 0.0f;
    };
}


int main(int argc, char *argv[])
{
    // Load Core
    gl::CoreCreateInfo core_spec = {"02-cube", 800, 600};
    gl::Core core(core_spec);

    // Load App
    game::App ctx(core);

    // Run App
    core.begin();

    SDL_Event event;
    while (core.running)
    {
        core.startFrame();

        while (SDL_PollEvent(&event))
        {
            core.update(event);
            ctx.on_event(event);
        }

        ctx.on_update(core.dt);
        ctx.on_render();

        core.endFrame();
    }

    return EXIT_SUCCESS;
}

game::App::App(gl::Core& core) : m_core(core)
{
    LOG_INFO("Building App");

    // === Load resources for Debug Renderer ===
    {
        cm::MappedFile prog_file("shaders/debug.shader.bin");
        cm::MappedFile data_file("models/primitives/base.primitives.bin");
    
        gl::ShaderProgramCreateInfo shader_params(prog_file, &temp_resource);
        gl::debug::primitives::ModelCacheCreateInfo cache_params(data_file, &temp_resource);
    
        m_debugPass = cm::allocate_unique<gl::debug::primitives::IRenderer, gl::debug::primitives::NaiveRenderer>(&main_resource, shader_params, cache_params);
    }

    // === Load resources for Quad Renderer ===
    {
        cm::MappedFile prog_file("shaders/quad.shader.bin");
        gl::ShaderProgramCreateInfo shader_params(prog_file, &temp_resource);

        m_quadPass = cm::allocate_unique<gl::debug::quads::IRenderer, gl::debug::quads::NaiveRenderer>(&main_resource, shader_params);
    }

    {
        m_camera = cm::allocate_unique<gl::Camera>(&main_resource);
        m_flybyController = cm::allocate_unique<game::FlyByCameraController>(&main_resource, m_core, *m_camera);
        m_ctaController = cm::allocate_unique<game::CTACameraController>(&main_resource, m_core, *m_flybyController);
    }

    // === Load resources for Texture ===
    {
        gl::Image image("images/hk.png", gl::ImageFormat::RGB);
        gl::TextureCreateInfo texture_params; // = {.internalFormat = GL_RGB8, .format = GL_RGB};
        m_texture = cm::allocate_unique<gl::Texture>(&main_resource, image, texture_params);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
}

game::App::~App() {}

void game::App::on_event(const SDL_Event& event) 
{
    m_ctaController->handleEvent(event);
}

void game::App::on_update(float dt) 
{
    m_ctaController->update(dt);
    time += dt;
} 

void game::App::on_render() 
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, time, glm::vec3(0.0f, 1.0f, 0.0f)); 

    glm::vec3 color = glm::vec3(0.0f, 0.0f, 1.0f);
    unsigned int cube = m_debugPass->model_id("cube");

    glm::mat4 model2 = glm::mat4(1.0f);  // identity
    model2 = glm::translate(model2, glm::vec3(0.0f, -2.0f, 0.0f));  // move down
    model2 = glm::scale(model2, glm::vec3(1000.0f));

    glm::vec3 color2 = glm::vec3(0.1f, 0.1f, 0.1f);
    unsigned int plane = m_debugPass->model_id("plane");

    gl::debug::quads::DrawCommand quad_cmd = {
        .pos = {-1.0f, 0.5f, .5f, .5f},  // Full screen quad
        .texCoord = {0.0f, 0.0f, 1.0f, 1.0f},  // Full texture coordinates
        .texture = m_texture->id  
    };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_debugPass->begin(gl::debug::primitives::FrameCommand{m_camera->getProjectionMatrix() * m_camera->getViewMatrix()});
    m_debugPass->submit(gl::debug::primitives::DrawCommand{model, color, cube});
    m_debugPass->submit(gl::debug::primitives::DrawCommand{model2, color2, plane});
    m_debugPass->end();

    m_quadPass->begin();
    m_quadPass->submit(quad_cmd);
    m_quadPass->end();
} 