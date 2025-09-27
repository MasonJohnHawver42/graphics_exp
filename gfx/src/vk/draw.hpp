#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/vec3.hpp>

#include "core.hpp"

struct Frame
{

};

struct Renderer 
{
    Core* instance;
};

struct DrawParams 
{
    glm::vec3 bg_color;
};

bool renderer_init(Renderer** state, SDL_Window* window);
void renderer_destroy(Renderer* state);

void draw_render(Renderer* state, DrawParams* params, float dt);

#endif // DRAW_H