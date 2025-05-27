#include <gl.h>
#include <cm.h>

#include <glad/glad.h>


class Application 
{
public:
    Application() : core({"01-window", 800, 600}) 
    {
    }

    void run() 
    {
        core.begin();
        SDL_Event event;
        while (core.running)
        {
            core.startFrame();

            while (SDL_PollEvent(&event))
            {
                core.update(event);
            }

            glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            core.endFrame();
        }
    }

private:
    gl::Core core;
};

int main(int argc, char *argv[])
{
    try
    {
        Application app;
        app.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Exception caught: %s", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}