#include "log.h"
#include "core.hpp"

#include <vulkan/vulkan.hpp>

#include "VkBootstrap.h"

#include <malloc.h>

//----------------------------------------------------------------------------//

#define START_MSG "hello world %d", 42

void hello_world(void) 
{
    MSG_LOG(START_MSG);
    ERROR_LOG(START_MSG);
}

//----------------------------------------------------------------------------//

static bool _core_create_vk_instance(Core* instance, SDL_Window* window, const char* name);
static void _core_destroy_vk_instance(Core* instance);;

static bool _core_create_swapchain(Core* instance, uint32_t w, uint32_t h);
static void _core_destroy_swapchain(Core* instance);

static bool _core_create_command_pool(Core* instance);
static void _core_destroy_command_pool(Core* instance);

//----------------------------------------------------------------------------//

bool core_init(Core** c, SDL_Window* window, const char* app_name)
{
    *c = (Core*)malloc(sizeof(Core));
	Core* core = *c;

    if(!_core_create_vk_instance(core, window, app_name)) 
    {
        ERROR_LOG("failed to create vk instance");
        return false;
    }

    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    if (w == 0 || h == 0) 
    {
        ERROR_LOG("failed to get window size");
        return false;
    }

    if(!_core_create_swapchain(core, w, h)) 
    {
        ERROR_LOG("failed to create a swapchain");
        return false;
    }

    if(!_core_create_command_pool(core)) 
    {
        ERROR_LOG("failed to create a command pool");
        return false;
    }

    return true;
}

void core_destroy(Core* core) 
{
    _core_destroy_command_pool(core);
    _core_destroy_swapchain(core);
    _core_destroy_vk_instance(core);

    free(core);
}

VkImageView core_create_image_view(Core* core, VkImage image, VkFormat format, VkImageAspectFlags aspects, uint32_t mipLevels) 
{
    VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspects;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView view;
	if(vkCreateImageView(core->device, &viewInfo, NULL, &view) != VK_SUCCESS) 
    {
		ERROR_LOG("failed to create image view");
    }

	return view;
}

void core_destroy_image_view(Core* core, VkImageView view) 
{
    vkDestroyImageView(core->device, view, NULL);
}


//----------------------------------------------------------------------------//

static bool _core_create_vk_instance(Core* core, SDL_Window* window, const char* name)
{
    MSG_LOG("create vk instance");

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name(name)
		.request_validation_layers(VKH_VALIDATION_LAYERS)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

    if (!inst_ret) 
    {
        ERROR_LOG("Failed to create Vulkan instance : %s", inst_ret.error().message().c_str());
        return false;
    }
    
    vkb::Instance vkb_inst = inst_ret.value();

    core->instance = vkb_inst.instance;

    #if VKH_VALIDATION_LAYERS
    {
        core->debugMessenger = vkb_inst.debug_messenger;
    }
    #endif

    SDL_Vulkan_CreateSurface(window, core->instance , &core->surface);

    //vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
	
    auto phys_ret = selector
		.set_surface(core->surface)
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
        // .require_dedicated_transfer_queue ()
		.select();
    
    if (!phys_ret) 
    {
        ERROR_LOG("Failed to select Vulkan Physical Device : %s", phys_ret.error().message().c_str());
        return false;
    }

    vkb::PhysicalDevice vkb_physicalDevice = phys_ret.value();

    vkb::DeviceBuilder device_builder{ vkb_physicalDevice };
    auto dev_ret = device_builder.build();

    if (!dev_ret) 
    {
        ERROR_LOG("Failed to create Vulkan device : %s", dev_ret.error().message().c_str());
        return false;
    }

    vkb::Device vkb_device = dev_ret.value();

	core->physicalDevice = vkb_physicalDevice.physical_device;
    core->device = vkb_device.device;

    auto graphics_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
    auto present_queue_ret = vkb_device.get_queue(vkb::QueueType::present);
    
    if (!graphics_queue_ret) 
    {
        ERROR_LOG("Failed to get graphics queue : %s", graphics_queue_ret.error().message().c_str());
        return false;
    }

    if (!present_queue_ret) 
    {
        ERROR_LOG("Failed to get present queue : %s", present_queue_ret.error().message().c_str());
        return false;
    }

    auto graphics_queue_index_ret = vkb_device.get_queue_index(vkb::QueueType::graphics);
    auto present_queue_index_ret = vkb_device.get_queue_index(vkb::QueueType::present);

    if (!graphics_queue_index_ret) 
    {
        ERROR_LOG("Failed to get graphics queue family index : %s", graphics_queue_index_ret.error().message().c_str());
        return false;
    }

    if (!present_queue_index_ret) 
    {
        ERROR_LOG("Failed to get present queue family index : %s", present_queue_index_ret.error().message().c_str());
        return false;
    }

    core->graphicsQueue = graphics_queue_ret.value();
    core->presentQueue = present_queue_ret.value();
    core->graphicsQueueFamilyIndex = graphics_queue_index_ret.value();
    core->presentQueueFamilyIndex = present_queue_index_ret.value();

    return true;
}

static void _core_destroy_vk_instance(Core* core) 
{
    MSG_LOG("destroy vk instance");

    vkDestroyDevice(core->device, NULL);

    vkDestroySurfaceKHR(core->instance, core->surface, NULL);

    #if VKH_VALIDATION_LAYERS
	{
		PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(core->instance, "vkDestroyDebugUtilsMessengerEXT");
		
		if(destroyDebugMessenger) 
        {
			destroyDebugMessenger(core->instance, core->debugMessenger, NULL);
        }
		else 
        {
		    ERROR_LOG("could not find function \"vkDestroyDebugUtilsMessengerEXT\"");
        }
	}
	#endif

    vkDestroyInstance(core->instance, NULL);
}

static bool _core_create_swapchain(Core* core, uint32_t w, uint32_t h) 
{
    MSG_LOG("create swapchain");

    // get format and present mode :

    uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(core->physicalDevice, core->surface, &formatCount, NULL);
	VkSurfaceFormatKHR* supportedFormats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(core->physicalDevice, core->surface, &formatCount, supportedFormats);

    VkSurfaceFormatKHR format = supportedFormats[0];
	for(uint32_t i = 0; i < formatCount; i++)
    {
		if(supportedFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && supportedFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			format = supportedFormats[i];
			break;
		}
    }

    free(supportedFormats);

    uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(core->physicalDevice, core->surface, &presentModeCount, NULL);
	VkPresentModeKHR* supportedPresentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(core->physicalDevice, core->surface, &presentModeCount, supportedPresentModes);
	
    VkPresentModeKHR presentMode = supportedPresentModes[0];
	for(uint32_t i = 0; i < presentModeCount; i++)
    {
		if(supportedPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
			presentMode = supportedPresentModes[i];
        }
    }

	free(supportedPresentModes);

    // get extent:

    VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core->physicalDevice, core->surface, &capabilities);

    VkExtent2D extent;
	if(capabilities.currentExtent.width != UINT32_MAX)
	{
		//TODO: does this work when the window is resized?
		extent = capabilities.currentExtent; //window already defined size for us
	}
    else
	{
		extent = (VkExtent2D){w, h};
		
		//clamping:
		extent.width = extent.width > capabilities.maxImageExtent.width ? capabilities.maxImageExtent.width : extent.width;
		extent.width = extent.width < capabilities.minImageExtent.width ? capabilities.minImageExtent.width : extent.width;

		extent.height = extent.height > capabilities.maxImageExtent.height ? capabilities.maxImageExtent.height : extent.height;
		extent.height = extent.height < capabilities.minImageExtent.height ? capabilities.minImageExtent.height : extent.height;
	}

    // get image count :

	uint32_t imageCount = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
		imageCount = capabilities.maxImageCount;
    }

    // get swapchain :
    
	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = NULL;
    swapchainInfo.flags = 0;
	swapchainInfo.surface = core->surface;
	swapchainInfo.minImageCount = imageCount;
	swapchainInfo.imageFormat = format.format;
	swapchainInfo.imageColorSpace = format.colorSpace;
	swapchainInfo.imageExtent = extent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.preTransform = capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;
	swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    uint32_t indices[] = {core->graphicsQueueFamilyIndex, core->presentQueueFamilyIndex};

    if(core->graphicsQueueFamilyIndex != core->presentQueueFamilyIndex)
	{	
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = indices;
	}
	else 
    {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if(vkCreateSwapchainKHR(core->device, &swapchainInfo, NULL, &core->swapchain) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create swapchain");
		return false;
	}

    core->swapchainExtent = extent;
	core->swapchainFormat = format.format;

    vkGetSwapchainImagesKHR(core->device, core->swapchain, &core->swapchainImageCount, NULL);
    
    core->swapchainImages = (VkImage*)malloc(core->swapchainImageCount * sizeof(VkImage));
	core->swapchainImageViews = (VkImageView*)malloc(core->swapchainImageCount * sizeof(VkImageView));

    vkGetSwapchainImagesKHR(core->device, core->swapchain, &core->swapchainImageCount, core->swapchainImages);

    for(uint32_t i = 0; i < core->swapchainImageCount; i++) 
    {
		core->swapchainImageViews[i] = core_create_image_view(core, core->swapchainImages[i], core->swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }

    return true;
}

static void _core_destroy_swapchain(Core* core) 
{
    MSG_LOG("destroy swapchain");

    for(uint32_t i = 0; i < core->swapchainImageCount; i++)
    {
		core_destroy_image_view(core, core->swapchainImageViews[i]);
    }

    free(core->swapchainImages);
	free(core->swapchainImageViews);

	vkDestroySwapchainKHR(core->device, core->swapchain, NULL);
}

static bool _core_create_command_pool(Core* core) 
{
    MSG_LOG("create command pool");

    VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = core->graphicsQueueFamilyIndex;

    if(vkCreateCommandPool(core->device, &poolInfo, NULL, &core->commandPool) != VK_SUCCESS)
	{
		ERROR_LOG("failed to create command pool");
		return false;
	}

	return true;
}

static void _core_destroy_command_pool(Core* core) 
{
    MSG_LOG("destroy command pool");

	vkDestroyCommandPool(core->device, core->commandPool, NULL);
}