#ifndef HEADERS_DEFINES_H_
#define HEADERS_DEFINES_H_


// This macro will include X11 header and Vulkan header
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_xlib.h>

#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <filesystem>


/* Uncomment to enable */
 #define VSYNC_MODE
// #define NDEBUG

const int MAX_FRAMES_IN_FLIGHT = 2;

// device level layers are deprecated and
// only instance level requests need to be made
const std::vector<const char *> requestedValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> requestedInstanceExtensions = {
    "VK_EXT_debug_utils",
    "VK_KHR_surface",
    "VK_KHR_xlib_surface"};

const std::vector<const char *> requestedDeviceExtensions = {
    "VK_KHR_swapchain",
    "VK_EXT_extended_dynamic_state"};

#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

// Contains Physical render device on system with Vulkan support
// Also contains other relevant info such as supported Queues etc
// Used as type for std vector deviceInfoList
struct DEVICEINFO
{
        VkPhysicalDevice devHandle = nullptr;

        int rating = 0; // used in selecting a device

        // index to queue family that has graphics command queue support
        uint32_t graphicsFamilyIndex = 0;
        // index to queue family with presentation support
        uint32_t presentFamilyIndex = 0;
        // how many types of command queues this device supports
        uint32_t queueFamilyCount = 0;
        // list of all queue families this device supports
        std::vector<VkQueueFamilyProperties> queueFamiles;

        VkPhysicalDeviceProperties2 devProperties{};
        VkPhysicalDeviceFeatures2 devFeatures{};
        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedFeatures{};
};

struct SwapChainSupportDetails
{
        VkSurfaceCapabilitiesKHR capabilities{};

        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
};

// CALLBACK FOR DEBUG UTILITIES
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageProcessor(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                     VkDebugUtilsMessageTypeFlagsEXT message_type,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                                                     void *user_data);


#endif