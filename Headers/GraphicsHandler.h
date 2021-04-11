// Copyright 2021 ... fake copy right
#ifndef HEADERS_GRAPHICSHANDLER_H_
#define HEADERS_GRAPHICSHANDLER_H_

// Includes most libraries needed
#include "Defines.h"

#include "MemoryHandler.h"
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
        std::vector<VkDescriptorSetLayout> m_DescriptorLayouts;
        // Model + View matrix set
        std::vector<VkDescriptorSetLayout> m_Set0Allocs;
        // Projection matrix set
        std::vector<VkDescriptorSetLayout> m_Set1Allocs;

        std::vector<VkDescriptorSet> m_ModelViewSets;
        std::vector<VkDescriptorSet> m_ProjectionSets;

        VkDebugUtilsMessengerEXT m_Debug = nullptr;
        SwapChainSupportDetails m_SurfaceDetails{};
        std::vector<VkImage> m_SwapImages;
        std::vector<VkImageView> m_SwapViews;
        std::vector<VkFramebuffer> m_Framebuffers;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        /* Buffers, Memory, Mapped ptrs */
        std::unique_ptr<MemoryHandler> memory;
        
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

        struct PipelineStageInfo
        {
                VkPipelineShaderStageCreateInfo vertexStageInfo{};
                VkPipelineShaderStageCreateInfo fragmentStageInfo{};

                VkShaderModule vertexModule = nullptr;
                VkShaderModule fragmentModule = nullptr;

                std::vector<VkAttachmentDescription> renderAttachments;
                std::vector<VkPipelineShaderStageCreateInfo> stageInfos;

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
                VkPipelineDynamicStateCreateInfo dynamicInfo{};
                VkViewport viewport{};
                VkRect2D scissor{};
                VkPipelineViewportStateCreateInfo viewportStateInfo{};
                VkPipelineRasterizationStateCreateInfo rasterInfo{};
                VkPipelineMultisampleStateCreateInfo samplingInfo{};
                VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
                VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo{};
                VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                VkAttachmentDescription colorAttachment{};
                VkAttachmentReference colorAttachmentRef{};
                VkSubpassDescription subpass{};
                VkSubpassDependency dependency{};
                VkRenderPassCreateInfo renderInfo{};

                VkGraphicsPipelineCreateInfo pipelineInfo{};

                std::vector<VkDynamicState> dStates = {VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT};

                std::vector<VkVertexInputBindingDescription> bindingDescription = Vertex::getBindingDescription();
                std::vector<VkVertexInputAttributeDescription> attributeDescription = Vertex::getAttributeDescriptions();
        } m_PipelineStageInfo;

private:
        void checkDeviceExtensionSupport(void);

        // If debugging enabled ensures debug layers found
        void checkValidationLayerSupport(void);

        // Ensures all requested extensions supported
        void checkInstanceExtensionSupport(void);

        // Calls all necessary functions to initialize
        void initVulkan(void);

        /*
         Creates Vulkan instance
         Loads debug utils if applicable
        */
        void createInstance(void);

        // Fetches all physical device handles and retrieves
        // properties and features for each of them
        void queryDevices(void);

        // Looks for best device to default to for rendering
        // Prioritizes discrete gpu and high image2d limits
        void selectAdapter(void);

        // Create's surface
        void createSurface(void);

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

        // Create logical device
        void createLogicalDevice(void);

        // Creates graphics/present queues
        // Typically graphics queue and present queue are the same index
        void createCommandQueues(void);

        // Create swapchain
        void createSwapChain(void);

        // Create views for returned swapchain images
        void createSwapViews(void);

        // Create layout for resource bindings
        void createDescriptorSetLayout(void);

        // Creates layout for pipeline
        void createPipelineLayout(void);

        // Create render pass
        void createRenderPass(void);

        // Create graphic pipeline
        void createGraphicsPipeline(void);

        // Create framebuffers
        void createFrameBuffers(void);

        void createCommandPool(void);

        void createCommandBuffers(void);

        void createSyncObjects(void);
        
        void createDescriptorPool(void);

        void createDescriptorSets(void);

        void bindDescriptorSets(void);
        
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                         VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);
        VkImageView createImageView(VkImage image, VkFormat);
        void loadEntities(void);
        void recordCommandBuffer(uint32_t imageIndex);

        void updateUniformModelBuffer(uint32_t imageIndex);
        void updateUniformVPBuffer(uint32_t imageIndex);

        static std::vector<char> readFile(std::string filename);
        VkExtent2D chooseSwapChainExtent(void);
        VkSurfaceFormatKHR chooseSwapChainFormat(void);
        VkPresentModeKHR chooseSwapChainPresentMode(void);
        VkShaderModule createShaderModule(const std::vector<char> &code);
        
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, int dstOffset, VkDeviceSize size);
        // Quick command buffer record start/end
        VkCommandBuffer beginSingleCommands(void);
        void endSingleCommands(VkCommandBuffer commandBuffer);

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
