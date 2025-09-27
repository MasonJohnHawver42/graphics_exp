#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <cm.h>
#include <gl.h>

#include <memory_resource>

struct Context 
{
    Context(std::pmr::memory_resource* mr);
    ~Context() = default;

    cm::Manual<gl::Core> core;
    cm::unique_ptr<gl::debug::primitives::IRenderer> debug_renderer;
    cm::unique_ptr<gl::debug::quads::IRenderer> quad_renderer;
};

extern Context* g_context;

#endif