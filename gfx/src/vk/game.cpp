#include "draw.hpp"
#include "game.hpp"
#include "log.h"

//----------------------------------------------------------------------------//

static bool _game_window_init(SDL_Window** window, uint32_t w, uint32_t h, const char* name);
static void _game_window_destroy(SDL_Window* window);

//----------------------------------------------------------------------------//

bool game_init(Game* game) 
{
    if(!_game_window_init(&game->window, 1920, 1080, "VkExp")) 
    {
        ERROR_LOG("window init failed"); 
        return false; 
    }

    if(!renderer_init(&game->renderer, game->window)) 
    {
        ERROR_LOG("core init failed");
        return false;
    }

    return true;
}

void game_destroy(Game* game) 
{
    renderer_destroy(game->renderer);
    _game_window_destroy(game->window);
}

//----------------------------------------------------------------------------//

static bool _game_window_init(SDL_Window** window, uint32_t w, uint32_t h, const char* name) 
{
    MSG_LOG("create window");

    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        ERROR_LOG("sdl2 init failed"); 
        return false;
    }

    if (SDL_Vulkan_LoadLibrary(nullptr) < 0) 
    {
        ERROR_LOG("sdl2 failed to load the Vulkan loader library"); 
        return false;
    }

    // Create window
    *window = SDL_CreateWindow(name, 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    if (!*window) 
    {
        ERROR_LOG("sdl2 window failed");
        SDL_Quit();
        return false;
    }

    return true;
}

static void _game_window_destroy(SDL_Window* window) 
{
    MSG_LOG("destroy window");

    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}