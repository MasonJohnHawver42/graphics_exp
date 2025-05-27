#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <cm.h>
#include <gl.h>

#include <memory_resource>

struct Context {
    Context(std::pmr::memory_resource* mr);
    ~Context() = default;

    cm::Manual<gl::Core> core;
    cm::unique_ptr<gl::debug::IRenderer> debug_renderer;
};

extern Context* g_context;

#endif