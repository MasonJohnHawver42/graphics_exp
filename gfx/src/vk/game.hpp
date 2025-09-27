#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "draw.hpp"

struct Game 
{
    SDL_Window* window;
    Renderer* renderer;
};

bool game_init(Game* game);
void game_destroy(Game* game);

#endif // GAME_H