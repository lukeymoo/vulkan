#ifndef HEADERS_MEMORYHANDLER_H_
#define HEADERS_MEMORYHANDLER_H_

#include "ExceptionHandler.h"
#include "Primitives.h"
#include "Defines.h"

struct MemoryInitParameters
{
    uint32_t vertexSize = 256;
    uint32_t indexSize = 256;
    VkPhysicalDevice *m_PhysicalDevice = nullptr;
    VkDevice *m_Device = nullptr;
    DEVICEINFO *selectedDevice = nullptr;
    SwapChainSupportDetails *m_SurfaceDetails = nullptr;
};

class MemoryHandler
{
public:
    class Exception : public ExceptionHandler
    {
    public:
        Exception(int l, std::string f, std::string message);
        ~Exception(void);
    };

public:
    MemoryHandler(void) = delete;
    MemoryHandler(const MemoryHandler &) = delete;
    MemoryHandler &operator=(const MemoryHandler &) = delete;

    MemoryHandler(MemoryInitParameters params);
    ~MemoryHandler(void);

private:
    MemoryInitParameters memVar;

    std::vector<VkImage> m_SwapImages;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformMemory;
    std::vector<void *> m_UniformPtrs;

    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexMemory;
    void *m_VertexPtr;

    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexMemory;
    void *m_IndexPtr;

private:
    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);
    void createVertexBuffer(void);
    void createIndexBuffer(void);

    void createUniformBuffers(void);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

#define M_EXCEPT(string) throw Exception(__LINE__, __FILE__, string);

#endif