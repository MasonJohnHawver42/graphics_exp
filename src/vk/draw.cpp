#include "core.hpp"
#include "draw.hpp"
#include "log.h"

#include <malloc.h>

//----------------------------------------------------------------------------//

bool renderer_init(Renderer** s, SDL_Window* window) 
{
    *s = (Renderer* )malloc(sizeof(Renderer));
	Renderer* state = *s;

    if(!core_init(&state->instance, window, "vk_exp")) 
    {
        ERROR_LOG("core init failed");
        return false;
    }

    return true;
}

void renderer_destroy(Renderer* state) 
{
    core_destroy(state->instance);

    free(state);
}

void draw_render(Renderer* state, DrawParams* params, float dt) 
{
    //todo
}