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

/* DEFINES */

/* Disable Debugging */

//#define NDEBUG

/* Present Modes */

#define PRESENT_MODE VK_PRESENT_MODE_IMMEDIATE_KHR
//#define PRESENT_MODE VK_PRESENT_MODE_MAILBOX_KHR
//#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
//#define PRESENT_MODE VK_PRESENT_MODE_FIFO_RELAXED_KHR

inline const char *presentModeToString(VkPresentModeKHR pmode)
{
    switch (pmode)
    {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return "IMMEDIATE KHR";
        break;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return "MAILBOX KHR";
        break;
    case VK_PRESENT_MODE_FIFO_KHR:
        return "FIFO KHR";
        break;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return "FIFO Relaxed KHR";
        break;
    default:
        return "Unknown Present Mode Specified";
        break;
    }
}

const int MAX_FRAMES_IN_FLIGHT = 2;

/*
    device level layers are deprecated and
    only instance level requests need to be made
*/
const std::vector<const char *> requestedValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> requestedInstanceExtensions = {
    "VK_EXT_debug_utils",
    "VK_KHR_surface",
    "VK_KHR_xlib_surface"};

const std::vector<const char *> requestedDeviceExtensions = {
    "VK_KHR_swapchain",
    "VK_EXT_extended_dynamic_state"};

// Contains Physical render device on system with Vulkan support
// Also contains other relevant info such as supported Queues etc
// Used as type for std vector deviceInfoList
struct DEVICEINFO
{
    VkPhysicalDevice devHandle = nullptr;

    int rating = 0; // used in selecting a device

    uint32_t graphicsFamilyIndex = 0;

    std::vector<uint32_t> presentIndexes;

    uint32_t queueFamilyCount = 0;
    std::vector<VkQueueFamilyProperties> queueFamiles;

    VkPhysicalDeviceProperties2 devProperties{};
    VkPhysicalDeviceFeatures2 devFeatures{};
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedFeatures{};
};

struct SwapChainSupportDetails
{
    VkSurfaceFormatKHR selectedFormat{};
    VkPresentModeKHR selectedPresentMode{};
    VkSharingMode sharingMode;
    VkSampleCountFlagBits selectedSampleCount = VK_SAMPLE_COUNT_1_BIT;

    VkSurfaceCapabilitiesKHR capabilities{};

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// CALLBACK FOR DEBUG UTILITIES
VKAPI_ATTR
VkBool32
    VKAPI_CALL
    debugMessageProcessor(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                          VkDebugUtilsMessageTypeFlagsEXT message_type,
                          const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                          void *user_data);

#endif