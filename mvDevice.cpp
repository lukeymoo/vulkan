#include "mvDevice.h"

namespace mv
{
    Device::Device(VkPhysicalDevice physicalDevice)
    {
        assert(physicalDevice);
    }

    Device::~Device(void)
    {
        if(m_CommandPool) {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        }
        if(m_Device) {
            vkDestroyDevice(m_Device, nullptr);
        }
    }

};