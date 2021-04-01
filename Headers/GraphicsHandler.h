// Copyright 2021 ... fake copy right
#ifndef HEADERS_GRAPHICSHANDLER_H_
#define HEADERS_GRAPHICSHANDLER_H_

// This macro will include X11 header and Vulkan header
#define VK_USE_PLATFORM_XLIB_KHR

#include <X11/Xutil.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <filesystem>

#include "ExceptionHandler.h"
#include "Primitives.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

// device level layers are deprecated and
// only instance level requests need to be made
const std::vector<const char*> requestedValidationLayers = {
"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> requestedInstanceExtensions = {
"VK_EXT_debug_utils",
"VK_KHR_surface",
"VK_KHR_xlib_surface"
};

const std::vector<const char*> requestedDeviceExtensions = {
"VK_KHR_swapchain"
};

#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

// Contains Physical render device on system with Vulkan support
// Also contains other relevant info such as supported Queues etc
// Used as type for std vector deviceInfoList
struct DEVICEINFO {
    VkPhysicalDevice devHandle = nullptr;

    int rating = 0;  // used in selecting a device

    // index to queue family that has graphics command queue support
    uint32_t graphicsFamilyIndex = 0;
    // index to queue family with presentation support
    uint32_t presentFamilyIndex = 0;
    // how many types of command queues this device supports
    uint32_t queueFamilyCount = 0;
    // list of all queue families this device supports
    std::vector<VkQueueFamilyProperties> queueFamiles;

    VkPhysicalDeviceProperties devProperties{};
    VkPhysicalDeviceFeatures devFeatures{};
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


// CALLBACK FOR DEBUG UTILITIES
VKAPI_ATTR
VkBool32
VKAPI_CALL
debugMessageProcessor(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data);




class GraphicsHandler {
    friend class WindowHandler;
    public:
        class Exception : public ExceptionHandler {
            public:
                Exception(int l, std::string f, std::string message);
                ~Exception(void);
        };

    public:
        GraphicsHandler(Display* dsp, Window* wnd, int w, int h);
        ~GraphicsHandler(void);

    public:
        // Helpers
        int getWindowWidth(void);
        int getWindowHeight(void);

        // makes all necessary calls to configure graphics pipeline
        void initGraphics(void);


    private:
        bool SHOULD_RENDER = true;
        int windowWidth;
        int windowHeight;
        // selects/configures logical device & command queue
        void initVulkan(void);
        void createInstance(void);
        // rate each device found for suitability
        bool selectAdapter(void);
        // creates surface and binds to windows api
        void registerSurface(void);
        void createLogicalDeviceAndQueues(void);
        /* Creates swap chain and views into swap images */
        void createSwapChain(void);
        void createDescriptorSetLayout(void);
        void createGraphicsPipeline(void);
        void createFrameBuffers(void);
        void createCommandPool(void);
        void createCommandBuffers(void);
        void createSyncObjects(void);
        void createVertexBuffer(void);
        void createIndexBuffer(void);
        void createUniformBuffer(void);
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory);
        void updateUniformBuffer(void);

        // Frees binded resources for recreation
        void cleanupSwapChain(void);
        // Used when window size changes ( entering/exiting fullscreen )
        void recreateSwapChain(void);

        /*
          -- HELPERS --
          Called internally by other functions
          Do not manually call to avoid duplicate resource creation
        */
        void createRenderPass(void);
        static std::vector<char> readFile(std::string filename);
        VkSurfaceFormatKHR chooseSwapChainFormat(void);
        VkPresentModeKHR chooseSwapChainPresentMode(void);
        VkExtent2D chooseSwapChainExtent(void);
        SwapChainSupportDetails querySwapChainSupport(void);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


        /*
          Function pointers used to call debug utilities extension functions
        */
        bool loadDebugUtils(void);  // loads the below function pointers
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
        PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT;

        /*
          If these functions fail they will return a list of all requests that failed
          for more comprehensive error reporting
        */
        // checks if specified layers are supported,
        // errors are stored at pointer as specified by parameter
        bool checkValidationLayerSupport(std::string* failList);
        // checks if specified extensions are supported,
        // errors are stored at pointer as specified by parameter
        bool checkInstanceExtensionSupport(std::string* failList);
        bool checkDeviceExtensionSupport(std::string* failList);

    private:
        uint32_t deviceCount = 0;
        const float queuePrio = 1.0f;

        // -- Vulkan objects --


        // the device handle is stored in deviceInfoList
        // vector as property `devHandle`
        // used selectedIndex as the index to retrieve appropriate
        // handle from vector
        VkPhysicalDevice m_PhysicalDevice;

        VkInstance m_Instance;  // handle to vulkan instance
        // logical device to interface with selected physical device
        VkDevice m_Device;
        VkQueue m_GraphicsQueue;  // graphics command queue handle
        // present queue <- most likely points to same queue as GraphicsQueue
        VkQueue m_PresentQueue;
        VkSurfaceKHR m_Surface;  // handle to window surface
        VkSwapchainKHR m_Swap;  // handle to swap chain
        VkCommandPool m_CommandPool;  // pool that holds commands to execute on gpu
        VkBuffer m_VertexBuffer;  // gpu local memory, not accessible with vkmap
        VkDeviceMemory m_VertexMemory;  // ditto
        VkBuffer m_IndexBuffer;
        VkDeviceMemory m_IndexMemory;

        /*
          -- Sync objects --
          Rendering will allow a constant ( 2 for now ) number of frames to be `in flight`
          to the screen

          Each of these frames will have their own semaphores/fences

        */
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;

        std::vector<VkImage> m_SwapImages;
        std::vector<VkImageView> m_SwapViews;
        std::vector<VkFramebuffer> m_Framebuffers;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        // Uniform buffers are updated on near per frame basis
        // Multiple are created to avoid updating data while a frame is still in flight
        // Uniform buffers contain data such as model, view and project matrices
        std::vector<VkBuffer> m_UniformBuffers;
        std::vector<VkDeviceMemory> m_UniformMemory;

        VkPipeline m_Pipeline;
        VkRenderPass m_RenderPass;
        VkPipelineLayout m_PipelineLayout;
        VkDescriptorSetLayout m_DescriptorLayout;


        // selected parameters that were supported by surface
        VkSurfaceFormatKHR selectedSwapFormat{};
        VkExtent2D selectedSwapExtent{};

        VkDebugUtilsMessengerEXT m_Debug;
        SwapChainSupportDetails m_SurfaceDetails;

        int selectedIndex = 0;  // index of the selected physical device we will use

        std::vector<DEVICEINFO> deviceInfoList;


        // will hold the structures defining all queues we want
        // needed for creation of logical device
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    private:
        Display* display;
        Window* window;
};

#define G_EXCEPT(string) \
    throw Exception(__LINE__, __FILE__, string); std::exit(-1);

#endif  // HEADERS_GRAPHICSHANDLER_H_
