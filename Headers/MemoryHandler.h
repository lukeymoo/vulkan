#ifndef HEADERS_MEMORYHANDLER_H_
#define HEADERS_MEMORYHANDLER_H_

#include "ExceptionHandler.h"
#include "Primitives.h"
#include "Defines.h"

struct MemoryInitParameters
{
    uint32_t vertexSize = 256;
    uint32_t indexSize = 256;
    VkPhysicalDevice &m_PhysicalDevice;
    VkDevice &m_Device;
    DEVICEINFO *selectedDevice;
    SwapChainSupportDetails &m_SurfaceDetails;

    MemoryInitParameters &operator=(MemoryInitParameters &n)
    {
        vertexSize = n.vertexSize;
        indexSize = n.indexSize;
        m_PhysicalDevice = n.m_PhysicalDevice;
        m_Device = n.m_Device;
        selectedDevice = n.selectedDevice;
        m_SurfaceDetails = n.m_SurfaceDetails;
    }
} memVar;

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

    MemoryHandler(MemoryInitParameters &params);
    ~MemoryHandler(void);

    VkBuffer *createUniformBuffer(void);
    VkDeviceMemory *getBufferMemory(VkBuffer *buf);
    void *getBufferPtr(VkBuffer *buf);

    void appendVertexData();

    void cleanup(void);

private:

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformMemory;
    std::vector<void *> m_UniformPtrs;

    VkBuffer m_VertexBuffer = nullptr;
    VkDeviceMemory m_VertexMemory = nullptr;
    void *m_VertexPtr = nullptr;

    VkBuffer m_IndexBuffer = nullptr;
    VkDeviceMemory m_IndexMemory = nullptr;
    void *m_IndexPtr = nullptr;

private:
    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);
    void createVertexBuffer(void);
    void createIndexBuffer(void);

    //std::vector<VkImage> m_SwapImages;
    //void createUniformBuffers(void);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

#define M_EXCEPT(string) throw Exception(__LINE__, __FILE__, string);

#endif