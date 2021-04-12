#ifndef HEADERS_MVDEVICE_H_
#define HEADERS_MVDEVICE_H_H

#include <cassert>
#include <vulkan/vulkan.h>

namespace mv
{
    struct Device
    {
        Device(VkPhysicalDevice physicalDevice);
        ~Device(void);

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;

        VkCommandPool m_CommandPool;

        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
    };
};

#endif