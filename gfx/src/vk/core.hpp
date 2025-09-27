#ifndef CORE_H
#define CORE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.hpp>

#define VKH_VALIDATION_LAYERS true

// chapter 0:
// ----------

void hello_world(void);

// chapter 1:
// ----------

typedef struct Core 
{   
    
    VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device; 
	VkSurfaceKHR surface;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
    uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;

    VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	uint32_t swapchainImageCount;
    VkImage* swapchainImages;
	VkImageView* swapchainImageViews;

    VkCommandPool commandPool;

    #if VKH_VALIDATION_LAYERS
        VkDebugUtilsMessengerEXT debugMessenger;
    #endif

} Core;


bool core_init(Core** inst, SDL_Window* window, const char* app_name);
void core_destroy(Core* instance);

VkImageView core_create_image_view(Core* core, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32_t mipLevels);
void core_destroy_image_view(Core* instance, VkImageView view);

#endif // CORE_H
