// Copyright 2021 ... fake copy right
#ifndef HEADERS_GRAPHICSHANDLER_H_
#define HEADERS_GRAPHICSHANDLER_H_

// Includes most libraries needed
#include "Defines.h"

#include "ExceptionHandler.h"
#include "Models.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Camera.h"

class GraphicsHandler
{
        friend class WindowHandler;

public:
        class Exception : public ExceptionHandler
        {
        public:
                Exception(int l, std::string f, std::string message);
                ~Exception(void);
        };

public:
        GraphicsHandler(Display *dsp, Window *wnd, int w, int h);
        ~GraphicsHandler(void);

public:
        // Helpers
        int getWindowWidth(void);
        int getWindowHeight(void);

        // makes all necessary calls to configure graphics pipeline
        void initGraphics(void);

private:
        Display *display;
        Window *window;
        int windowWidth = 0;
        int windowHeight = 0;
        bool SHOULD_RENDER = true;

        int selectedIndex = 0;
        int MAX_UNIFORM_BUFFER_SIZE = 0;

        std::unique_ptr<Camera> camera;

        /*
                Base objects/pipeline objects
        */
        VkPhysicalDevice m_PhysicalDevice = nullptr;
        VkInstance m_Instance = nullptr;
        VkDevice m_Device = nullptr;
        VkQueue m_GraphicsQueue = nullptr;
        VkQueue m_PresentQueue = nullptr;
        VkSurfaceKHR m_Surface = nullptr;
        VkSwapchainKHR m_Swap = nullptr;
        VkCommandPool m_CommandPool = nullptr;
        VkPipeline m_Pipeline = nullptr;
        VkRenderPass m_RenderPass = nullptr;
        VkPipelineLayout m_PipelineLayout = nullptr;
        VkDescriptorPool m_DescriptorPool = nullptr;
        VkDescriptorSetLayout m_DescriptorLayout = nullptr;
        VkDebugUtilsMessengerEXT m_Debug = nullptr;
        VkExtent2D selectedSwapExtent{};
        VkSurfaceFormatKHR selectedSwapFormat{};
        SwapChainSupportDetails m_SurfaceDetails{};
        std::vector<VkImage> m_SwapImages;
        std::vector<VkImageView> m_SwapViews;
        std::vector<VkFramebuffer> m_Framebuffers;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkDescriptorSet> m_DescriptorSets;

        /* Buffers, Memory, Mapped ptrs */
        VkBuffer m_VertexBuffer = nullptr;
        VkDeviceMemory m_VertexMemory = nullptr;
        VkBuffer m_IndexBuffer = nullptr;
        VkDeviceMemory m_IndexMemory = nullptr;
        std::vector<VkBuffer> m_UniformModelBuffers;
        std::vector<VkDeviceMemory> m_UniformModelMemories;
        std::vector<void *> m_UniformModelPtrs;

        std::vector<VkBuffer> m_UniformVPBuffers;
        std::vector<VkDeviceMemory> m_UniformVPMemories;
        std::vector<void *> m_UniformVPPtrs;

        /* Configured after a device is selected */
        DEVICEINFO *selectedDevice = nullptr;
        std::vector<DEVICEINFO> deviceInfoList;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        // Synchronization objects
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;

        /* Rendered Objects */
        ModelClass Human;

        /* Rendered Debug Objects */
        uint gridEndOffset = 0;
        uint gridStartOffset = 0;
        uint gridVertexDataSize = 0;
        std::vector<Vertex> grid;

private:
        // Calls all necessary functions to initialize
        void initVulkan(void);

        /*
         Creates Vulkan instance and loads debug utils if
         applicable
        */
        void createInstance(void);

        // Fetches all physical device handles and retrieves
        // properties and features for each of them
        void queryDevices(void);

        // Looks for best device to default to for rendering
        // Prioritizes discrete gpu and high local memory
        bool selectAdapter(void);

        // Queries device queue families and indexes them
        // in selectedDevice->presentIndexes
        void findPresentSupport(void);

        // Generates QueueCreateInfos for graphics queue
        // and separate present queue if necessary
        // adds create infos to queueCreateInfos
        void configureCommandQueues(void);

        // Queries device for swapchain support
        // stores in m_SurfaceDetails
        void configureSwapChain(void);

        // Queries selected device surface supported
        // formats, capabilities and present modes
        // Stores in m_SurfaceDetails
        void querySwapChainSupport(void);

        void createLogicalDeviceAndQueues(void);
        void createSwapChain(void);
        void createDescriptorSetLayout(void);
        void createGraphicsPipeline(void);
        void createFrameBuffers(void);
        void createCommandPool(void);
        void createCommandBuffers(void);
        void createSyncObjects(void);
        void createVertexBuffer(void);
        void createIndexBuffer(void);
        void createUniformBuffers(void);
        void createDescriptorPool(void);
        void createDescriptorSets(void);
        void createRenderPass(void);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer &buffer, VkDeviceMemory &bufferMemory);
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        VkImageView createImageView(VkImage image, VkFormat);
        void loadEntities(void);
        void recordCommandBuffer(uint32_t imageIndex);

        void updateUniformModelBuffer(uint32_t imageIndex);
        void updateUniformVPBuffer(uint32_t imageIndex);

        /*
                -- HELPERS --
                Called internally
        */
        static std::vector<char> readFile(std::string filename);
        VkExtent2D chooseSwapChainExtent(void);
        VkSurfaceFormatKHR chooseSwapChainFormat(void);
        VkPresentModeKHR chooseSwapChainPresentMode(void);
        VkShaderModule createShaderModule(const std::vector<char> &code);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, int dstOffset, VkDeviceSize size);
        // Quick command buffer record start/end
        VkCommandBuffer beginSingleCommands(void);
        void endSingleCommands(VkCommandBuffer commandBuffer);

        bool checkDeviceExtensionSupport(std::string *failList);
        bool checkValidationLayerSupport(std::string *failList);
        bool checkInstanceExtensionSupport(std::string *failList);

        void cleanupSwapChain(void);
        void recreateSwapChain(void);
        void processModelData(ModelClass *modelObj);

        /*
                  Function pointers used to call debug utilities extension functions
                */
        bool loadDebugUtils(void); // loads the below function pointers
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
        PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT = nullptr;

        // Functions provided by device level extensions
        bool loadDevicePFN(void);
        PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT = nullptr;

        void createGridVertices(void);
        std::pair<int, int> loadGridVertices(std::pair<int, int> prevOffsets);
        void processGridData(void);
};

#define G_EXCEPT(string) throw Exception(__LINE__, __FILE__, string);

#endif // HEADERS_GRAPHICSHANDLER_H_
