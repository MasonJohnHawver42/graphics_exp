#ifndef CAMERA_H
#define CAMERA_H

#include <gl.h>
#include <SDL2/SDL.h>

#include "context.h"

namespace game 
{
    class ICameraController 
    {
    public:
        ICameraController(gl::Camera* camera = nullptr) : cam(camera) {}

        virtual ~ICameraController() = default;
    
        virtual void update(float dt) = 0;
        virtual void handleEvent(const SDL_Event& event) = 0;
    
        virtual void setCamera(gl::Camera* camera) { cam = camera; }
        virtual gl::Camera* getCamera() const { return cam; }

    protected:
        gl::Camera* cam;
    };
    
    class FlyByCameraController : public ICameraController 
    {
        public:
        
        FlyByCameraController(gl::Camera* camera = nullptr) : ICameraController(camera) 
        {
            if(cam) 
            {
                int width, height;
                SDL_GetWindowSize(g_context->core->m_window, &width, &height);

                cam->aspect = static_cast<float>(width) / static_cast<float>(height);
            }
        }
    
        void handleEvent(const SDL_Event& e) 
        {
            if (!cam) return; // Ensure camera is set

            if (e.type == SDL_MOUSEMOTION) 
            {
                float dx = static_cast<float>(e.motion.xrel);
                float dy = static_cast<float>(e.motion.yrel);
    
                glm::quat yaw = glm::angleAxis(-dx * sensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::quat pitch = glm::angleAxis(-dy * sensitivity, glm::vec3(1.0f, 0.0f, 0.0f));
                cam->orientation = glm::normalize(yaw * cam->orientation * pitch);
            }
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                int width, height;
                SDL_GetWindowSize(g_context->core->m_window, &width, &height);
                cam->aspect = static_cast<float>(width) / static_cast<float>(height);
            }
        }
    
        void update(float dt) 
        {
            if (!cam) return; // Ensure camera is set

            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            glm::vec3 forward = cam->orientation * glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 right = cam->orientation * glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 up = cam->orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    
            if (keystate[SDL_SCANCODE_W]) cam->position += forward * moveSpeed * dt;
            if (keystate[SDL_SCANCODE_S]) cam->position -= forward * moveSpeed * dt;
            if (keystate[SDL_SCANCODE_A]) cam->position -= right * moveSpeed * dt;
            if (keystate[SDL_SCANCODE_D]) cam->position += right * moveSpeed * dt;
            if (keystate[SDL_SCANCODE_E]) cam->position += up * moveSpeed * dt;
            if (keystate[SDL_SCANCODE_Q]) cam->position -= up * moveSpeed * dt;
        }
    
        float moveSpeed = 5.0f;
        float sensitivity = 0.003f;
    };

    class CTACameraController : public ICameraController {
        public:
            enum State {
                Inactive,
                Active
            };
        
            CTACameraController(ICameraController* wrapped)
                : ICameraController(wrapped->getCamera()), wrappedController(wrapped) {}
        
            void handleEvent(const SDL_Event& event) override {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                    toggleState();
                }
        
                if (state == State::Active) {
                    wrappedController->handleEvent(event);
                }
            }
        
            void update(float dt) override {
                if (state == State::Active) {
                    wrappedController->update(dt);
                }
            }

            void setCamera(gl::Camera* camera) 
            {
                cam = camera;
                wrappedController->setCamera(camera);
            }
        
        private:
            void toggleState() {
                if (state == State::Inactive) {
                    SDL_ShowCursor(SDL_DISABLE);
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    // Optional: warp to center
                    int w, h;
                    SDL_GetWindowSize(g_context->core->m_window, &w, &h);
                    SDL_WarpMouseInWindow(g_context->core->m_window, w / 2, h / 2);
                    state = State::Active;
                } else {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    SDL_ShowCursor(SDL_ENABLE);
                    state = State::Inactive;
                }
            }
        
            ICameraController* wrappedController;
            State state = State::Inactive;
        };

}

#endif