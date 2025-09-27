#include <cm.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <gl.h>

#include "game.h"

Context* g_context = nullptr;

int main(int argc, char *argv[])
{
    std::byte main_buffer[1024 * 10];
    std::pmr::monotonic_buffer_resource main_resource{main_buffer, 1024 * 10};

    try
    {
        Context ctx(&main_resource);
        g_context = &ctx;

        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 color = glm::vec3(0.0f, 0.0f, 1.0f);
        unsigned int cube = ctx.debug_renderer->model_id("cube");

        glm::mat4 model2 = glm::mat4(1.0f);  // identity
        model2 = glm::translate(model2, glm::vec3(0.0f, -2.0f, 0.0f));  // move down
        model2 = glm::scale(model2, glm::vec3(1000.0f));
        glm::vec3 color2 = glm::vec3(0.1f, 0.1f, 0.1f);
        unsigned int plane = ctx.debug_renderer->model_id("plane");

        cm::Manual<gl::Texture> texture;

        {
            gl::Image image("images/hk.png", gl::ImageFormat::RGB);
            gl::TextureCreateInfo texture_params; // = {.internalFormat = GL_RGB8, .format = GL_RGB};
            texture.construct(image, texture_params);
        }

        cm::MappedFile prog_file("models/pbr/base.texture_pool.bin");
        gl::pbr::TextureCacheCreateInfo test_tcci(prog_file);

        // STREAM_INFO << test_tcci;

        gl::Camera camera;
        game::FlyByCameraController camera_controller(&camera);
        game::CTACameraController cta_controller(&camera_controller);

        gl::debug::quads::DrawCommand quad_cmd = {
            .pos = {-1.0f, 0.5f, .5f, .5f},  // Full screen quad
            .texCoord = {0.0f, 0.0f, 1.0f, 1.0f},  // Full texture coordinates
            .texture = texture->id  
        };
        
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    
        ctx.core->begin();
        
        SDL_Event event;
        while (ctx.core->running)
        {
            ctx.core->startFrame();

            while (SDL_PollEvent(&event))
            {
                ctx.core->update(event);
                cta_controller.handleEvent(event);
            }

            cta_controller.update(ctx.core->dt);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ctx.debug_renderer->begin(gl::debug::primitives::FrameCommand{camera.getProjectionMatrix() * camera.getViewMatrix()});
            ctx.debug_renderer->submit(gl::debug::primitives::DrawCommand{model, color, cube});
            ctx.debug_renderer->submit(gl::debug::primitives::DrawCommand{model2, color2, plane});
            ctx.debug_renderer->end();

            
            ctx.quad_renderer->begin();
            ctx.quad_renderer->submit(quad_cmd);
            ctx.quad_renderer->end();

            ctx.core->endFrame();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Exception caught\n\n%s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

Context::Context(std::pmr::memory_resource* mr) {
    // === Initialize Core ===
    {
        gl::CoreCreateInfo core_params = {"01-window", 800, 600};
        core.construct(core_params);
    }

    // === Load resources for Debug Renderer ===
    {
        cm::MappedFile prog_file("shaders/debug.shader.bin");
        cm::MappedFile data_file("models/primitives/base.primitives.bin");
    
        gl::ShaderProgramCreateInfo shader_params(prog_file, mr);
        gl::debug::primitives::ModelCacheCreateInfo cache_params(data_file, mr);
    
        debug_renderer = cm::allocate_unique<gl::debug::primitives::IRenderer, gl::debug::primitives::NaiveRenderer>(mr, shader_params, cache_params);
    }

    // === Load resources for Quad Renderer ===
    {
        cm::MappedFile prog_file("shaders/quad.shader.bin");

        gl::ShaderProgramCreateInfo shader_params(prog_file, mr);

        quad_renderer = cm::allocate_unique<gl::debug::quads::IRenderer, gl::debug::quads::NaiveRenderer>(mr, shader_params);
    }
}