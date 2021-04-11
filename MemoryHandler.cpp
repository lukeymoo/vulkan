#include "MemoryHandler.h"

MemoryHandler::MemoryHandler(MemoryInitParameters params)
{
    memVar = params;
    createVertexBuffer();
    createIndexBuffer();
}

MemoryHandler::~MemoryHandler()
{
}

void MemoryHandler::createVertexBuffer(void)
{
    std::cout << "[+] Creating vertex buffer" << std::endl;
    VkDeviceSize bufferSize = memVar.vertexSize;

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_VertexBuffer,
                 m_VertexMemory);
    return;
}

void MemoryHandler::createIndexBuffer(void)
{
    std::cout << "[+] Creating index buffer" << std::endl;
    // Index buffer size
    VkDeviceSize bufferSize = memVar.indexSize;

    // Create the gpu local buffer and copy our staging buffer to it
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_IndexBuffer,
                 m_IndexMemory);
    return;
}

void MemoryHandler::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = memVar.m_SurfaceDetails->sharingMode;
    if (memVar.m_SurfaceDetails->sharingMode == VK_SHARING_MODE_EXCLUSIVE)
    {
        bufferInfo.queueFamilyIndexCount = 1;
        bufferInfo.pQueueFamilyIndices = &memVar.selectedDevice->graphicsFamilyIndex;
    }
    else if (memVar.m_SurfaceDetails->sharingMode == VK_SHARING_MODE_CONCURRENT)
    {
        uint32_t indices[] = {memVar.selectedDevice->graphicsFamilyIndex, memVar.selectedDevice->presentIndexes[0]};
        bufferInfo.queueFamilyIndexCount = 2;
        bufferInfo.pQueueFamilyIndices = indices;
    }

    if (vkCreateBuffer(*memVar.m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        M_EXCEPT("Failed to create vertex buffer!");
    }

    // allocate memory for buffer
    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(*memVar.m_Device, buffer, &memRequirements);

    // Describe memory allocation
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(*memVar.m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        M_EXCEPT("Failed to allocate memory for vertex buffer!");
    }

    if (vkBindBufferMemory(*memVar.m_Device, buffer, bufferMemory, 0) != VK_SUCCESS)
    {
        M_EXCEPT("Failed to bind memory to vertex buffer");
    }
    return;
}

uint32_t MemoryHandler::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(*memVar.m_PhysicalDevice,
                                        &deviceMemoryProperties);

    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) &&
            (deviceMemoryProperties.memoryTypes[i].propertyFlags &
             properties) == properties)
        {
            return i;
        }
    }
    M_EXCEPT("Failed to find memory type");
}

void MemoryHandler::createUniformBuffers(void)
{
    // Create uniform buffer
    std::cout << "[+] Creating uniform buffers" << std::endl;
    
    m_UniformBuffers.resize(m_SwapImages.size());
    m_UniformMemory.resize(m_SwapImages.size());
    m_UniformPtrs.resize(m_SwapImages.size());

    for (const auto &image : m_SwapImages)
    {
        createBuffer(memVar.selectedDevice->devProperties.properties.limits.maxUniformBufferRange,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                     m_UniformBuffers[&image - &m_SwapImages[0]],
                     m_UniformMemory[&image - &m_SwapImages[0]]);

        if (vkMapMemory(*memVar.m_Device,
                        m_UniformMemory[&image - &m_SwapImages[0]],
                        0,
                        VK_WHOLE_SIZE,
                        0,
                        &m_UniformPtrs[&image - &m_SwapImages[0]]) != VK_SUCCESS)
        {
            M_EXCEPT("Failed to map uniform buffer");
        }
    }
    return;
}