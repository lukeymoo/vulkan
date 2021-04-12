// Copyright 2021 - not a real copyright, cpplint was annoying me
#include "GraphicsHandler.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GraphicsHandler::Exception::Exception(int l, std::string f, std::string description)
    : ExceptionHandler(l, f, description)
{
  type = "Graphics Handler Exception";
  errorDescription = description;
  return;
}

GraphicsHandler::Exception::~Exception(void)
{
  return;
}

GraphicsHandler::GraphicsHandler(Display *dsp, Window *wnd, int w, int h)
    : display(dsp), window(wnd), windowWidth(w), windowHeight(h), Human("Human")
{
  return;
}

void GraphicsHandler::initGraphics(void)
{
#ifndef NDEBUG
  std::cout << "[!] Debugging enabled" << std::endl;
  std::cout << "\t[-] Checking for validation layers" << std::endl;

  checkValidationLayerSupport();

  std::cout << "\t[+] All layers found" << std::endl;
#endif

  // Ensure system supports requested instance extensions here
  std::cout << "[+] Checking instance level extension support" << std::endl;
  checkInstanceExtensionSupport();

  initVulkan();

  return;
}

void GraphicsHandler::checkValidationLayerSupport(void)
{
  uint32_t layerCount = 0;
  if (vkEnumerateInstanceLayerProperties(&layerCount, nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query supported instance layer count");
  }

  if (layerCount == 0 && !requestedValidationLayers.empty())
  {
    G_EXCEPT("No supported validation layers found");
  }

  std::vector<VkLayerProperties> availableLayers(layerCount);
  if (vkEnumerateInstanceLayerProperties(&layerCount,
                                         availableLayers.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query supported instance layer list");
  }

  std::string prelude = "The following instance layers were not found...\n";
  std::string failed;
  for (const auto &requestedLayer : requestedValidationLayers)
  {
    bool match = false;
    for (const auto &availableLayer : availableLayers)
    {
      if (strcmp(requestedLayer, availableLayer.layerName) == 0)
      {
        match = true;
        break;
      }
    }
    if (!match)
    {
      failed += requestedLayer + '\n';
    }
  }

  if (!failed.empty())
  {
    G_EXCEPT(prelude + failed);
  }

  return;
}

void GraphicsHandler::checkInstanceExtensionSupport(void)
{
  uint32_t instanceExtensionCount = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr,
                                             &instanceExtensionCount,
                                             nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query instance supported extension count");
  }

  if (instanceExtensionCount == 0 && !requestedInstanceExtensions.empty())
  {
    G_EXCEPT("No instance level extensions supported by device");
  }

  std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
  if (vkEnumerateInstanceExtensionProperties(nullptr,
                                             &instanceExtensionCount,
                                             availableInstanceExtensions.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query instance supported extensions list");
  }

  std::string prelude = "The following instance extensions were not found...\n";
  std::string failed;
  for (const auto &requestedExtension : requestedInstanceExtensions)
  {
    bool match = false;
    for (const auto &availableExtension : availableInstanceExtensions)
    {
      // check if match
      if (strcmp(requestedExtension, availableExtension.extensionName) == 0)
      {
        match = true;
        break;
      }
    }

    if (!match)
    {
      failed += requestedExtension + '\n';
    }
  }

  if (!failed.empty())
  {
    G_EXCEPT(prelude + failed);
  }

  return;
}

void GraphicsHandler::initVulkan(void)
{
  /*
    Creates Vulkan instance and if debugging is enabled
    will load debug utility functions
  */
  std::cout << "[+] Creating instance" << std::endl;
  createInstance();

  // Fetches devices and their info
  std::cout << "[+] Querying system devices" << std::endl;
  queryDevices();

  /*
    Selects an adapter we will create a logical device for
    in the process it also retrieves the index of graphics queue family
    and stores as member variable of deviceInfoList[selectedIndex]
  */
  std::cout << "[+] Selecting adapter" << std::endl;
  selectAdapter();

  // Store physical device handle in m_PhysicalDevice
  m_PhysicalDevice = deviceInfoList.at(selectedIndex).devHandle;

  // Create window surface
  std::cout << "[+] Creating surface" << std::endl;
  createSurface();

  // Fetches list of queues that support presentation
  std::cout << "[+] Searching for presentation support" << std::endl;
  findPresentSupport();

  // Creates QueueCreateInfo structs if necessary a Present queue
  std::cout << "[+] Configuring command queue allocation" << std::endl;
  configureCommandQueues();

  /* Fetch swap chain info for creation */
  std::cout << "[+] Querying swapchain support details" << std::endl;
  querySwapChainSupport();

  // Check device level extension support
  std::cout << "[+] Checking device level extension support" << std::endl;
  checkDeviceExtensionSupport();

  // Create logical device
  std::cout << "[+] Creating logical device" << std::endl;
  createLogicalDevice();

  // Create command queues (present + graphics)
  std::cout << "[+] Creating command queues" << std::endl;
  createCommandQueues();

  // Load our device level PFN functions
  std::cout << "[+] Loading dynamic state functions" << std::endl;
  loadDevicePFN();

  std::cout << "[+] Creating swapchain" << std::endl;
  createSwapChain();

  std::cout << "[+] Creating swap views" << std::endl;
  createSwapViews();

  // Create descriptor set layout
  std::cout << "[+] Creating descriptor layout" << std::endl;
  createDescriptorSetLayout();

  // Create pipeline layout
  std::cout << "[+] Creating pipeline layout" << std::endl;
  createPipelineLayout();

  // Create render pass
  std::cout << "[+] Creating render pass" << std::endl;
  createRenderPass();

  // Create the pipeline
  std::cout << "[+] Creating graphics pipeline" << std::endl;
  createGraphicsPipeline();

  // Create frame buffers
  std::cout << "[+] Creating frame buffers" << std::endl;
  createFrameBuffers();

  // Create command pool
  std::cout << "[+] Allocating for command pool" << std::endl;
  createCommandPool();

  /*
    These are buffers that a specified queue family's commands are recorded in
    This allocates memory from a parent command pool
  */
  std::cout << "[+] Allocating for command buffer" << std::endl;
  createCommandBuffers();

  /*
    Memory allocator handles vertex, index and uniform buffers
  */
  MemoryInitParameters params = {
      params.vertexSize = 256,
      params.indexSize = 256,
      params.m_PhysicalDevice = m_PhysicalDevice,
      params.m_Device = m_Device,
      params.selectedDevice = selectedDevice,
      params.m_SurfaceDetails = m_SurfaceDetails};

  memory = std::make_unique<MemoryHandler>(params);

#ifndef NDEBUG
  std::cout << "[+] Creating grid vertices" << std::endl;
  createGridVertices(); // Does not move into memory
#endif

  /*
    Loads all defined models' data into vertex, index and MVP buffers
  */
  // std::cout << "[+] Loading models..." << std::endl;
  // loadEntities();

  // Temp
  // Add a single human element to type container
  //Human.humans.push_back(HumanClass());

  /*
   Describes the constraints on allocation of descriptor sets
   type, number/size etc
  */
  std::cout << "[+] Creating descriptor pool" << std::endl;
  createDescriptorPool();

  /*
    Allocates descriptor sets from the preallocated pool
  */
  std::cout << "[+] Allocating descriptor sets" << std::endl;
  createDescriptorSets();

  // Binds our created descriptors with resources
  std::cout << "[+] Binding descriptor sets" << std::endl;
  bindDescriptorSets();

  /*
    Used to synchronize operation

    So that commands are not executed when resources are not yet available
    For ex: pipeline images
  */
  std::cout << "[+] Creating synchronization resources" << std::endl;
  createSyncObjects();

  // Create the camera
  camera = std::make_unique<Camera>(m_SurfaceDetails.capabilities.currentExtent.width, m_SurfaceDetails.capabilities.currentExtent.height);

  return;
}

void GraphicsHandler::createInstance(void)
{
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = nullptr;
  appInfo.pApplicationName = "Bloody Day";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Moogin";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

/* If debugging enabled */
#ifndef NDEBUG
  VkDebugUtilsMessengerCreateInfoEXT debuggerSettings{};
  debuggerSettings.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debuggerSettings.pNext = nullptr;
  debuggerSettings.flags = 0;
  debuggerSettings.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debuggerSettings.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debuggerSettings.pfnUserCallback = debugMessageProcessor;
  debuggerSettings.pUserData = nullptr;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext = &debuggerSettings;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount = static_cast<uint32_t>(requestedValidationLayers.size());
  createInfo.ppEnabledLayerNames = requestedValidationLayers.data();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedInstanceExtensions.size());
  createInfo.ppEnabledExtensionNames = requestedInstanceExtensions.data();
#endif
#ifdef NDEBUG
  /* Debugging disabled */
  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledLayerCount = static_cast<uint32_t>(requestedValidationLayers.size());
  createInfo.ppEnabledLayerNames = requestedValidationLayers.data();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedInstanceExtensions.size());
  createInfo.ppEnabledExtensionNames = requestedInstanceExtensions.data();
#endif

  // Create instance
  if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create Vulkan instance!");
  }

/* Debugging enabled */
#ifndef NDEBUG

  // Load debugging utilities and create messenger
  loadDebugUtils();
  if (vkCreateDebugUtilsMessengerEXT(m_Instance, &debuggerSettings, nullptr, &m_Debug) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create debug messenger");
  }
#endif

  return;
}

// Fetches all physical device handles and retrieves
// properties and features for each of them
void GraphicsHandler::queryDevices(void)
{
  uint32_t numDevices = 0;
  if (vkEnumeratePhysicalDevices(m_Instance, &numDevices, nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failure enumerating devices");
  }

  if (numDevices == 0)
  {
    G_EXCEPT("No adapters with Vulkan support found");
  }

  deviceInfoList.resize(numDevices);
  std::vector<VkPhysicalDevice> tempContainer(numDevices);

  // Fetch devices
  if (vkEnumeratePhysicalDevices(m_Instance, &numDevices, tempContainer.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failure fetching physical device handles");
  }

  // Move device handles to custom structure
  for (size_t i = 0; i < deviceInfoList.size(); i++)
  {
    deviceInfoList.at(i).devHandle = tempContainer.at(i);
  }

  // Populate our container with devices and their properties/features
  for (auto &deviceContainer : deviceInfoList)
  {
    memset(&deviceContainer.devProperties, 0, sizeof(VkPhysicalDeviceProperties2));
    memset(&deviceContainer.devFeatures, 0, sizeof(VkPhysicalDeviceFeatures2));
    memset(&deviceContainer.extendedFeatures, 0, sizeof(VkPhysicalDeviceExtendedDynamicStateFeaturesEXT));
    // Configure structures so that Vulkan will recognize and populate them
    deviceContainer.devProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceContainer.devFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceContainer.extendedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    // Append extendedFeatures container to devFeatures structure
    deviceContainer.devFeatures.pNext = &deviceContainer.extendedFeatures;

    // Fetch properties/features for each device
    vkGetPhysicalDeviceProperties2(deviceContainer.devHandle, &deviceContainer.devProperties);
    vkGetPhysicalDeviceFeatures2(deviceContainer.devHandle, &deviceContainer.devFeatures);
    // Fetch queue families
    vkGetPhysicalDeviceQueueFamilyProperties(deviceContainer.devHandle, &deviceContainer.queueFamilyCount, nullptr);
    if (deviceContainer.queueFamilyCount == 0)
    {
      deviceContainer.rating = -1;
      continue;
    }
    deviceContainer.queueFamiles.resize(deviceContainer.queueFamilyCount, {});
    vkGetPhysicalDeviceQueueFamilyProperties(deviceContainer.devHandle, &deviceContainer.queueFamilyCount,
                                             deviceContainer.queueFamiles.data());
  }
  return;
}

void GraphicsHandler::selectAdapter(void)
{
  for (auto &deviceContainer : deviceInfoList)
  {
    // If rating is -1, skip
    if (deviceContainer.rating == -1)
    {
      continue;
    }

    if (deviceContainer.devProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      deviceContainer.rating += 1000;
    }
    deviceContainer.rating += deviceContainer.devProperties.properties.limits.maxImageDimension2D;

    // Iterate device's queue families and ensure graphics is supported
    bool hasGraphics = false;
    for (auto &queue : deviceContainer.queueFamiles)
    {
      if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        hasGraphics = true;
        deviceContainer.graphicsFamilyIndex = (&queue - &deviceContainer.queueFamiles[0]);
      }
    } // end queue loop

    if (!hasGraphics || !deviceContainer.devFeatures.features.geometryShader)
    {
      deviceContainer.rating = 0;
      continue;
    }
  } // end container loop

  /* -- SELECTION -- */
  selectedIndex = 0;
  for (auto &device : deviceInfoList)
  {
    if (device.rating > deviceInfoList.at(selectedIndex).rating)
    {
      selectedIndex = (&device - &deviceInfoList[0]);
    }
  }

  if (deviceInfoList.at(selectedIndex).rating <= 0)
  {
    G_EXCEPT("Failed to find a suitable adapter");
  }

  selectedDevice = &deviceInfoList.at(selectedIndex);

  return;
}

void GraphicsHandler::createSurface(void)
{
  VkXlibSurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = nullptr;
  createInfo.flags = 0;
  createInfo.dpy = display;
  createInfo.window = *window;

  // Create window surface
  std::cout << "[+] Creating Vulkan surface" << std::endl;
  if (vkCreateXlibSurfaceKHR(m_Instance, &createInfo, nullptr, &m_Surface) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create surface");
  }
  return;
}

void GraphicsHandler::findPresentSupport(void)
{
  for (auto &queue : selectedDevice->queueFamiles)
  {
    VkBool32 canPresent = false;
    uint32_t currentIndex = (&queue - &deviceInfoList.at(selectedIndex).queueFamiles[0]);
    if (vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice,
                                             currentIndex,
                                             m_Surface,
                                             &canPresent) != VK_SUCCESS)
    {
      G_EXCEPT("There was an error determining if queue families support presentation");
    }
    if (canPresent)
    {
      deviceInfoList.at(selectedIndex).presentIndexes.push_back(currentIndex);
    }
  }

  if (deviceInfoList.at(selectedIndex).presentIndexes.empty())
  {
    G_EXCEPT("Selected device does not support presentation");
  }
  return;
}

void GraphicsHandler::configureCommandQueues(void)
{
  // Double check we have present support
  if (selectedDevice->presentIndexes.empty())
  {
    G_EXCEPT("Presentation not supported on selected device");
  }

  // Graphics queue
  VkDeviceQueueCreateInfo graphicsQueueInfo{};
  const float prio[] = {1.0f};
  graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  graphicsQueueInfo.pNext = nullptr;
  graphicsQueueInfo.flags = 0;
  graphicsQueueInfo.queueFamilyIndex = selectedDevice->graphicsFamilyIndex;
  graphicsQueueInfo.queueCount = 1;
  graphicsQueueInfo.pQueuePriorities = prio;

  queueCreateInfos.push_back(graphicsQueueInfo);

  // See if graphics queue also supports presentation
  bool hasPresent = false;
  for (auto &index : selectedDevice->presentIndexes)
  {
    if (selectedDevice->graphicsFamilyIndex == index)
    {
      hasPresent = true;
    }
  }

  // Describe another queue index as presentation
  if (!hasPresent)
  {
    VkDeviceQueueCreateInfo presentQueueInfo{};
    const float prio[] = {1.0f};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.pNext = nullptr;
    graphicsQueueInfo.flags = 0;
    graphicsQueueInfo.queueFamilyIndex = selectedDevice->presentIndexes[0];
    graphicsQueueInfo.queueCount = 1;
    graphicsQueueInfo.pQueuePriorities = prio;

    queueCreateInfos.push_back(presentQueueInfo);
  }
  return;
}

void GraphicsHandler::querySwapChainSupport(void)
{
  // Get surface capabilities
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice,
                                                m_Surface,
                                                &m_SurfaceDetails.capabilities) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query surface capabilities");
  }

  // Get surface formats
  uint32_t formatCount = 0;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice,
                                           m_Surface, &formatCount, nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query count of surface formats");
  }
  if (formatCount == 0)
  {
    G_EXCEPT("No supported formats for surface");
  }
  m_SurfaceDetails.formats.resize(formatCount);
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice,
                                           m_Surface,
                                           &formatCount,
                                           m_SurfaceDetails.formats.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query surface formats list");
  }

  // Get present modes
  uint32_t presentModeCount = 0;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice,
                                                m_Surface,
                                                &presentModeCount,
                                                nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query present mode count");
  }
  if (presentModeCount == 0)
  {
    G_EXCEPT("Failed to find any supported present modes");
  }
  m_SurfaceDetails.presentModes.resize(presentModeCount);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice,
                                                m_Surface,
                                                &presentModeCount,
                                                m_SurfaceDetails.presentModes.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query present mode list");
  }

  // Double check formats/present modes
  if (m_SurfaceDetails.formats.empty())
  {
    G_EXCEPT("Selected device has no supported formats");
  }
  if (m_SurfaceDetails.presentModes.empty())
  {
    G_EXCEPT("Selected device has no supported present modes");
  }
  return;
}

void GraphicsHandler::checkDeviceExtensionSupport(void)
{
  uint32_t deviceExtensionCount = 0;
  if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice,
                                           nullptr,
                                           &deviceExtensionCount,
                                           nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query device supported extension count");
  }

  if (deviceExtensionCount == 0 && !requestedDeviceExtensions.empty())
  {
    G_EXCEPT("Device reporting no supported extensions");
  }

  std::vector<VkExtensionProperties> availableExtensions(deviceExtensionCount);
  if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice,
                                           nullptr,
                                           &deviceExtensionCount,
                                           availableExtensions.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query device supported extension list");
  }

  // Check `requestedDeviceExtensions` against
  // availableExtensions
  std::string prelude = "The following device extensions were not found...\n";
  std::string failed;
  for (const auto &requestedExtension : requestedDeviceExtensions)
  {
    bool match = false;
    for (const auto &availableExtension : availableExtensions)
    {
      if (strcmp(requestedExtension, availableExtension.extensionName) == 0)
      {
        match = true;
        break;
      }
    }
    if (!match)
    {
      failed += requestedExtension + '\n';
    }
  }
  if (!failed.empty())
  {
    G_EXCEPT(prelude + failed);
  }
  return;
}

void GraphicsHandler::createLogicalDevice(void)
{
  VkDeviceCreateInfo deviceCreateInfo{};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pNext = &selectedDevice->devFeatures;
  deviceCreateInfo.flags = 0;
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledLayerNames = nullptr;
  deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();
  deviceCreateInfo.pEnabledFeatures = nullptr;

  // Create device
  if (vkCreateDevice(m_PhysicalDevice,
                     &deviceCreateInfo,
                     nullptr,
                     &m_Device) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create logical device");
  }
  return;
}

void GraphicsHandler::createCommandQueues(void)
{
  // Graphics queue
  vkGetDeviceQueue(m_Device,
                   selectedDevice->graphicsFamilyIndex,
                   0,
                   &m_GraphicsQueue);

  m_PresentQueue = m_GraphicsQueue;

  // Only larger than 1 if separate present was found to be necessary
  if (queueCreateInfos.size() > 1)
  {
    vkGetDeviceQueue(m_Device,
                     selectedDevice->presentIndexes[0],
                     0,
                     &m_PresentQueue);
  }

  // Ensure we created queues
  if (!m_GraphicsQueue || !m_PresentQueue)
  {
    G_EXCEPT("Failed to create command queues");
  }
  return;
}

// Device level extension functions
bool GraphicsHandler::loadDevicePFN(void)
{
  PFN_vkVoidFunction fp;

  fp = vkGetDeviceProcAddr(m_Device, "vkCmdSetPrimitiveTopologyEXT");
  if (!fp)
  {
    G_EXCEPT("Failed to load device level extension function vkCmdSetPrimitiveTopologyEXT");
  }
  vkCmdSetPrimitiveTopologyEXT = reinterpret_cast<PFN_vkCmdSetPrimitiveTopologyEXT>(fp);

  return true;
}

void GraphicsHandler::createSwapChain(void)
{
  m_SurfaceDetails.selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  for (const auto &mode : m_SurfaceDetails.presentModes)
  {
    if (mode == PRESENT_MODE)
    {
      m_SurfaceDetails.selectedPresentMode = mode;
    }
  }

  m_SurfaceDetails.selectedFormat = m_SurfaceDetails.formats[0];

  for (const auto &format : m_SurfaceDetails.formats)
  {
    if (format.format == VK_FORMAT_B8G8R8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      m_SurfaceDetails.selectedFormat = format;
    }
  }

  // Same queue family doing present/graphics does not need concurrent
  std::vector<uint32_t> queueIndices;

  queueIndices.push_back(selectedDevice->graphicsFamilyIndex);

  if (m_PresentQueue == m_GraphicsQueue)
  {
    m_SurfaceDetails.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  else
  {
    m_SurfaceDetails.sharingMode = VK_SHARING_MODE_CONCURRENT;
    queueIndices.push_back(selectedDevice->presentIndexes[0]);
  }

  VkSwapchainCreateInfoKHR swapInfo{};
  swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapInfo.pNext = nullptr;
  swapInfo.flags = 0;
  swapInfo.surface = m_Surface;
  swapInfo.minImageCount = m_SurfaceDetails.capabilities.maxImageCount;
  swapInfo.imageFormat = m_SurfaceDetails.selectedFormat.format;
  swapInfo.imageColorSpace = m_SurfaceDetails.selectedFormat.colorSpace;
  swapInfo.imageExtent = m_SurfaceDetails.capabilities.currentExtent;
  swapInfo.imageArrayLayers = 1;
  swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapInfo.imageSharingMode = m_SurfaceDetails.sharingMode;
  swapInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueIndices.size());
  swapInfo.pQueueFamilyIndices = queueIndices.data();
  swapInfo.preTransform = m_SurfaceDetails.capabilities.currentTransform;
  swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapInfo.presentMode = m_SurfaceDetails.selectedPresentMode;
  swapInfo.clipped = VK_TRUE;
  swapInfo.oldSwapchain = nullptr;

  if (vkCreateSwapchainKHR(m_Device,
                           &swapInfo,
                           nullptr,
                           &m_Swap) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create swapchain");
  }

  return;
}

void GraphicsHandler::createSwapViews(void)
{
  /* Retrieve swapchain images */
  uint32_t imageCount = 0;
  if (vkGetSwapchainImagesKHR(m_Device,
                              m_Swap,
                              &imageCount, nullptr) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to query swapchain image count");
  }
  if (imageCount == 0)
  {
    G_EXCEPT("Swapchain creation did not produce any images");
  }

  m_SwapImages.resize(imageCount);
  if (vkGetSwapchainImagesKHR(m_Device,
                              m_Swap,
                              &imageCount,
                              m_SwapImages.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to retrieve swapchain images");
  }

  /* Create views into swapchain images */
  m_SwapViews.resize(imageCount);
  for (const auto &image : m_SwapImages)
  {
    m_SwapViews.push_back(
        createImageView(image, m_SurfaceDetails.selectedFormat.format));
  }
  return;
}

void GraphicsHandler::createDescriptorSetLayout(void)
{
  /* Set 0 */
  // Model matrix binding -- Per object
  VkDescriptorSetLayoutBinding modelBinding{};
  modelBinding.binding = 0;
  modelBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  modelBinding.descriptorCount = 1;
  modelBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  modelBinding.pImmutableSamplers = nullptr;

  // View matrix binding -- per scene
  VkDescriptorSetLayoutBinding viewBinding{};
  viewBinding.binding = 1;
  viewBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  viewBinding.descriptorCount = 1;
  viewBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  viewBinding.pImmutableSamplers = nullptr;

  /* Set 1 */
  // Projection matrix binding -- changes when extent changes
  VkDescriptorSetLayoutBinding projBinding{};
  projBinding.binding = 0;
  projBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  projBinding.descriptorCount = 1;
  projBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  projBinding.pImmutableSamplers = nullptr;

  std::vector<VkDescriptorSetLayoutBinding> set0Binding = {modelBinding, viewBinding};
  std::vector<VkDescriptorSetLayoutBinding> set1Binding = {projBinding};

  VkDescriptorSetLayoutCreateInfo set0Info{};
  set0Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set0Info.pNext = nullptr;
  set0Info.flags = 0;
  set0Info.bindingCount = static_cast<uint32_t>(set0Binding.size());
  set0Info.pBindings = set0Binding.data();

  VkDescriptorSetLayoutCreateInfo set1Info{};
  set1Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set1Info.pNext = nullptr;
  set1Info.flags = 0;
  set1Info.bindingCount = static_cast<uint32_t>(set1Binding.size());
  set1Info.pBindings = set1Binding.data();

  std::vector<VkDescriptorSetLayoutCreateInfo> layouts = {set0Info, set1Info};

  m_DescriptorLayouts.resize(static_cast<uint32_t>(layouts.size()));

  for (const auto &layout : layouts)
  {
    if (vkCreateDescriptorSetLayout(m_Device,
                                    &layout,
                                    nullptr,
                                    &m_DescriptorLayouts[(&layout - &layouts[0])]) != VK_SUCCESS)
    {
      G_EXCEPT("Failed to create descriptor layout");
    }
  }
  return;
}

void GraphicsHandler::createPipelineLayout(void)
{
  auto vertexBlob = readFile("shaders/vert.spv");
  auto fragmentBlob = readFile("shaders/frag.spv");

  m_PipelineStageInfo.vertexModule = createShaderModule(vertexBlob);
  m_PipelineStageInfo.fragmentModule = createShaderModule(fragmentBlob);

  m_PipelineStageInfo.vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  m_PipelineStageInfo.vertexStageInfo.pNext = nullptr;
  m_PipelineStageInfo.vertexStageInfo.flags = 0;
  m_PipelineStageInfo.vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  m_PipelineStageInfo.vertexStageInfo.module = m_PipelineStageInfo.vertexModule;
  m_PipelineStageInfo.vertexStageInfo.pName = "main";
  m_PipelineStageInfo.vertexStageInfo.pSpecializationInfo = nullptr;

  m_PipelineStageInfo.fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  m_PipelineStageInfo.fragmentStageInfo.pNext = nullptr;
  m_PipelineStageInfo.fragmentStageInfo.flags = 0;
  m_PipelineStageInfo.fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  m_PipelineStageInfo.fragmentStageInfo.module = m_PipelineStageInfo.fragmentModule;
  m_PipelineStageInfo.fragmentStageInfo.pName = "main";
  m_PipelineStageInfo.fragmentStageInfo.pSpecializationInfo = nullptr;

  m_PipelineStageInfo.stageInfos = {m_PipelineStageInfo.vertexStageInfo, m_PipelineStageInfo.fragmentStageInfo};

  m_PipelineStageInfo.vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  m_PipelineStageInfo.vertexInputInfo.pNext = nullptr;
  m_PipelineStageInfo.vertexInputInfo.flags = 0;
  m_PipelineStageInfo.vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_PipelineStageInfo.bindingDescription.size());
  m_PipelineStageInfo.vertexInputInfo.pVertexBindingDescriptions = m_PipelineStageInfo.bindingDescription.data();
  m_PipelineStageInfo.vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_PipelineStageInfo.attributeDescription.size());
  m_PipelineStageInfo.vertexInputInfo.pVertexAttributeDescriptions = m_PipelineStageInfo.attributeDescription.data();

  /*
    Ignored when pDynamicState is set in VkPipelineCreateInfo
  */
  // m_PipelineStageInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  // m_PipelineStageInfo.inputAssemblyInfo.pNext = nullptr;
  // m_PipelineStageInfo.inputAssemblyInfo.flags = 0;
  // m_PipelineStageInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  // m_PipelineStageInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  m_PipelineStageInfo.dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  m_PipelineStageInfo.dynamicInfo.pNext = nullptr;
  m_PipelineStageInfo.dynamicInfo.flags = 0;
  m_PipelineStageInfo.dynamicInfo.dynamicStateCount = static_cast<uint32_t>(m_PipelineStageInfo.dStates.size());
  m_PipelineStageInfo.dynamicInfo.pDynamicStates = m_PipelineStageInfo.dStates.data();

  m_PipelineStageInfo.viewport.x = 0;
  m_PipelineStageInfo.viewport.y = 0;
  m_PipelineStageInfo.viewport.width = static_cast<float>(m_SurfaceDetails.capabilities.currentExtent.width);
  m_PipelineStageInfo.viewport.height = static_cast<float>(m_SurfaceDetails.capabilities.currentExtent.height);
  m_PipelineStageInfo.viewport.minDepth = 0.0f;
  m_PipelineStageInfo.viewport.maxDepth = 1.0f;

  m_PipelineStageInfo.scissor.offset = {0, 0};
  m_PipelineStageInfo.scissor.extent = m_SurfaceDetails.capabilities.currentExtent;

  m_PipelineStageInfo.viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  m_PipelineStageInfo.viewportStateInfo.pNext = nullptr;
  m_PipelineStageInfo.viewportStateInfo.flags = 0;
  m_PipelineStageInfo.viewportStateInfo.viewportCount = 1;
  m_PipelineStageInfo.viewportStateInfo.pViewports = &m_PipelineStageInfo.viewport;
  m_PipelineStageInfo.viewportStateInfo.scissorCount = 1;
  m_PipelineStageInfo.viewportStateInfo.pScissors = &m_PipelineStageInfo.scissor;

  m_PipelineStageInfo.rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  m_PipelineStageInfo.rasterInfo.pNext = nullptr;
  m_PipelineStageInfo.rasterInfo.flags = 0;
  m_PipelineStageInfo.rasterInfo.depthClampEnable = VK_FALSE;
  m_PipelineStageInfo.rasterInfo.rasterizerDiscardEnable = VK_FALSE;
  m_PipelineStageInfo.rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  m_PipelineStageInfo.rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  m_PipelineStageInfo.rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  m_PipelineStageInfo.rasterInfo.depthBiasEnable = VK_FALSE;
  m_PipelineStageInfo.rasterInfo.depthBiasConstantFactor = 0.0f;
  m_PipelineStageInfo.rasterInfo.depthBiasClamp = 0.0f;
  m_PipelineStageInfo.rasterInfo.depthBiasSlopeFactor = 0.0f;
  m_PipelineStageInfo.rasterInfo.lineWidth = 1.0f;

  m_PipelineStageInfo.samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  m_PipelineStageInfo.samplingInfo.pNext = nullptr;
  m_PipelineStageInfo.samplingInfo.flags = 0;
  m_PipelineStageInfo.samplingInfo.rasterizationSamples = m_SurfaceDetails.selectedSampleCount;
  m_PipelineStageInfo.samplingInfo.sampleShadingEnable = VK_FALSE;
  m_PipelineStageInfo.samplingInfo.minSampleShading = 1.0f;
  m_PipelineStageInfo.samplingInfo.pSampleMask = nullptr;
  m_PipelineStageInfo.samplingInfo.alphaToCoverageEnable = VK_FALSE;
  m_PipelineStageInfo.samplingInfo.alphaToOneEnable = VK_FALSE;

  m_PipelineStageInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  m_PipelineStageInfo.depthStencilInfo.pNext = nullptr;
  m_PipelineStageInfo.depthStencilInfo.flags = 0;
  m_PipelineStageInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
  m_PipelineStageInfo.depthStencilInfo.depthWriteEnable = VK_FALSE;
  m_PipelineStageInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  m_PipelineStageInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
  m_PipelineStageInfo.depthStencilInfo.front = {};
  m_PipelineStageInfo.depthStencilInfo.back = {};
  m_PipelineStageInfo.depthStencilInfo.minDepthBounds = 0.0f;
  m_PipelineStageInfo.depthStencilInfo.maxDepthBounds = 1.0f;

  m_PipelineStageInfo.colorBlendAttachmentInfo.blendEnable = VK_TRUE;
  m_PipelineStageInfo.colorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  m_PipelineStageInfo.colorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  m_PipelineStageInfo.colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
  m_PipelineStageInfo.colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  m_PipelineStageInfo.colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  m_PipelineStageInfo.colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
  m_PipelineStageInfo.colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                                VK_COLOR_COMPONENT_G_BIT |
                                                                VK_COLOR_COMPONENT_B_BIT |
                                                                VK_COLOR_COMPONENT_A_BIT;

  m_PipelineStageInfo.colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  m_PipelineStageInfo.colorBlendingInfo.pNext = nullptr;
  m_PipelineStageInfo.colorBlendingInfo.flags = 0;
  m_PipelineStageInfo.colorBlendingInfo.logicOpEnable = VK_FALSE;
  m_PipelineStageInfo.colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
  m_PipelineStageInfo.colorBlendingInfo.attachmentCount = 1;
  m_PipelineStageInfo.colorBlendingInfo.pAttachments = &m_PipelineStageInfo.colorBlendAttachmentInfo;
  m_PipelineStageInfo.colorBlendingInfo.blendConstants[0] = 0.0f;
  m_PipelineStageInfo.colorBlendingInfo.blendConstants[1] = 0.0f;
  m_PipelineStageInfo.colorBlendingInfo.blendConstants[2] = 0.0f;
  m_PipelineStageInfo.colorBlendingInfo.blendConstants[3] = 0.0f;

  m_PipelineStageInfo.pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  m_PipelineStageInfo.pipelineLayoutInfo.pNext = nullptr;
  m_PipelineStageInfo.pipelineLayoutInfo.flags = 0;
  m_PipelineStageInfo.pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_DescriptorLayouts.size());
  m_PipelineStageInfo.pipelineLayoutInfo.pSetLayouts = m_DescriptorLayouts.data();
  m_PipelineStageInfo.pipelineLayoutInfo.pushConstantRangeCount = 0;
  m_PipelineStageInfo.pipelineLayoutInfo.pPushConstantRanges = nullptr;

  // Create pipeline layout
  if (vkCreatePipelineLayout(m_Device,
                             &m_PipelineStageInfo.pipelineLayoutInfo,
                             nullptr,
                             &m_PipelineLayout) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create pipeline layout");
  }

  return;
}

void GraphicsHandler::createRenderPass(void)
{
  m_PipelineStageInfo.colorAttachment.flags = 0;
  m_PipelineStageInfo.colorAttachment.format = m_SurfaceDetails.selectedFormat.format;
  m_PipelineStageInfo.colorAttachment.samples = m_SurfaceDetails.selectedSampleCount;
  /* Specify behavior of color/depth buffer before/after rendering */
  m_PipelineStageInfo.colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  m_PipelineStageInfo.colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  /* Specify behavior of stencil buffer before/after rendering */
  m_PipelineStageInfo.colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  m_PipelineStageInfo.colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  m_PipelineStageInfo.colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  m_PipelineStageInfo.colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  m_PipelineStageInfo.renderAttachments = {m_PipelineStageInfo.colorAttachment};

  m_PipelineStageInfo.colorAttachmentRef.attachment = 0;
  m_PipelineStageInfo.colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  m_PipelineStageInfo.subpass.flags = 0;
  m_PipelineStageInfo.subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  m_PipelineStageInfo.subpass.inputAttachmentCount = 0;
  m_PipelineStageInfo.subpass.pInputAttachments = nullptr;
  m_PipelineStageInfo.subpass.colorAttachmentCount = 1;
  m_PipelineStageInfo.subpass.pColorAttachments = &m_PipelineStageInfo.colorAttachmentRef;
  m_PipelineStageInfo.subpass.pResolveAttachments = nullptr;
  m_PipelineStageInfo.subpass.pDepthStencilAttachment = nullptr;
  m_PipelineStageInfo.subpass.preserveAttachmentCount = 0;
  m_PipelineStageInfo.subpass.pPreserveAttachments = nullptr;

  m_PipelineStageInfo.dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  m_PipelineStageInfo.dependency.dstSubpass = 0;
  m_PipelineStageInfo.dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  m_PipelineStageInfo.dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  m_PipelineStageInfo.dependency.srcAccessMask = 0;
  m_PipelineStageInfo.dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  m_PipelineStageInfo.renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  m_PipelineStageInfo.renderInfo.pNext = nullptr;
  m_PipelineStageInfo.renderInfo.flags = 0;
  m_PipelineStageInfo.renderInfo.attachmentCount = static_cast<uint32_t>(m_PipelineStageInfo.renderAttachments.size());
  m_PipelineStageInfo.renderInfo.pAttachments = m_PipelineStageInfo.renderAttachments.data();
  m_PipelineStageInfo.renderInfo.subpassCount = 1;
  m_PipelineStageInfo.renderInfo.pSubpasses = &m_PipelineStageInfo.subpass;
  m_PipelineStageInfo.renderInfo.dependencyCount = 1;
  m_PipelineStageInfo.renderInfo.pDependencies = &m_PipelineStageInfo.dependency;

  if (vkCreateRenderPass(m_Device,
                         &m_PipelineStageInfo.renderInfo,
                         nullptr,
                         &m_RenderPass) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create render pass");
  }
  return;
}

void GraphicsHandler::createGraphicsPipeline(void)
{
  m_PipelineStageInfo.pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  m_PipelineStageInfo.pipelineInfo.pNext = nullptr;
  m_PipelineStageInfo.pipelineInfo.flags = 0;
  m_PipelineStageInfo.pipelineInfo.stageCount = static_cast<uint32_t>(m_PipelineStageInfo.stageInfos.size());
  m_PipelineStageInfo.pipelineInfo.pStages = m_PipelineStageInfo.stageInfos.data();
  m_PipelineStageInfo.pipelineInfo.pVertexInputState = &m_PipelineStageInfo.vertexInputInfo;
  //m_PipelineStageInfo.pipelineInfo.pInputAssemblyState = &m_PipelineStageInfo.inputAssemblyInfo;
  // nullptr when using dynamic state
  m_PipelineStageInfo.pipelineInfo.pInputAssemblyState = nullptr;
  m_PipelineStageInfo.pipelineInfo.pTessellationState = nullptr;
  m_PipelineStageInfo.pipelineInfo.pViewportState = &m_PipelineStageInfo.viewportStateInfo;
  m_PipelineStageInfo.pipelineInfo.pRasterizationState = &m_PipelineStageInfo.rasterInfo;
  m_PipelineStageInfo.pipelineInfo.pMultisampleState = &m_PipelineStageInfo.samplingInfo;
  m_PipelineStageInfo.pipelineInfo.pDepthStencilState = &m_PipelineStageInfo.depthStencilInfo;
  m_PipelineStageInfo.pipelineInfo.pColorBlendState = &m_PipelineStageInfo.colorBlendingInfo;
  m_PipelineStageInfo.pipelineInfo.pDynamicState = &m_PipelineStageInfo.dynamicInfo;
  m_PipelineStageInfo.pipelineInfo.layout = m_PipelineLayout;
  m_PipelineStageInfo.pipelineInfo.renderPass = m_RenderPass;
  m_PipelineStageInfo.pipelineInfo.subpass = 0;
  m_PipelineStageInfo.pipelineInfo.basePipelineHandle = nullptr;
  m_PipelineStageInfo.pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(m_Device,
                                nullptr,
                                1,
                                &m_PipelineStageInfo.pipelineInfo,
                                nullptr,
                                &m_Pipeline) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create graphics pipeline");
  }

  vkDestroyShaderModule(m_Device, m_PipelineStageInfo.vertexModule, nullptr);
  vkDestroyShaderModule(m_Device, m_PipelineStageInfo.fragmentModule, nullptr);
  return;
}

void GraphicsHandler::createFrameBuffers(void)
{
  m_Framebuffers.resize(m_SwapViews.size());

  for (const auto &view : m_SwapViews)
  {
    std::vector<VkImageView> attachments = {view};
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.pNext = nullptr;
    fbInfo.flags = 0;
    fbInfo.renderPass = m_RenderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = m_SurfaceDetails.capabilities.currentExtent.width;
    fbInfo.height = m_SurfaceDetails.capabilities.currentExtent.height;
    fbInfo.layers = 1;
    if (vkCreateFramebuffer(m_Device,
                            &fbInfo,
                            nullptr,
                            &m_Framebuffers[(&view - &m_SwapViews[0])]) != VK_SUCCESS)
    {
      G_EXCEPT("Failed to create framebuffer");
    }
  }

  return;
}

void GraphicsHandler::createCommandPool(void)
{
  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.pNext = nullptr;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolInfo.queueFamilyIndex = selectedDevice->graphicsFamilyIndex;

  if (vkCreateCommandPool(m_Device,
                          &commandPoolInfo,
                          nullptr,
                          &m_CommandPool) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create graphics command pool");
  }
  return;
}

void GraphicsHandler::createCommandBuffers(void)
{
  VkResult result;

  // allocate command buffer for each framebuffer
  m_CommandBuffers.resize(m_Framebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

  result = vkAllocateCommandBuffers(m_Device,
                                    &allocInfo,
                                    m_CommandBuffers.data());
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to allocate command buffers!");
  }

  return;
}

void GraphicsHandler::createSyncObjects(void)
{
  VkResult result;

  VkSemaphoreCreateInfo semaphoreCreateInfo{};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphoreCreateInfo.pNext = nullptr;
  semaphoreCreateInfo.flags = 0;

  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.pNext = nullptr;
  // by default created in unsignaled state which
  // causes a hang at start of draw portion of game loop
  // due to waiting for a fence that was never submitted
  // so we use this flag to simulate that
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  // Create semaphores for each swap chain image
  m_imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  m_imagesInFlight.resize(m_SwapImages.size(), VK_NULL_HANDLE);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    result = vkCreateSemaphore(m_Device,
                               &semaphoreCreateInfo,
                               nullptr,
                               &m_imageAvailableSemaphore[i]);
    if (result != VK_SUCCESS)
    {
      G_EXCEPT("Failed to create image semaphore!");
    }

    result = vkCreateSemaphore(m_Device,
                               &semaphoreCreateInfo,
                               nullptr,
                               &m_renderFinishedSemaphore[i]);
    if (result != VK_SUCCESS)
    {
      G_EXCEPT("Failed to create render semaphore");
    }

    result = vkCreateFence(m_Device,
                           &fenceCreateInfo,
                           nullptr,
                           &m_inFlightFences[i]);
    if (result != VK_SUCCESS)
    {
      G_EXCEPT("Failed to create fences");
    }
  }
  return;
}

void GraphicsHandler::createDescriptorPool(void)
{
  VkResult result;

  VkDescriptorPoolSize modelPool{};
  modelPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  modelPool.descriptorCount = static_cast<uint32_t>(m_SwapImages.size());

  VkDescriptorPoolSize viewPool{};
  viewPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  viewPool.descriptorCount = static_cast<uint32_t>(m_SwapImages.size());

  VkDescriptorPoolSize projPool{};
  projPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  projPool.descriptorCount = static_cast<uint32_t>(m_SwapImages.size());

  // Container for the pool size structs
  std::vector<VkDescriptorPoolSize> poolDescriptors = {modelPool, viewPool, projPool};

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.pNext = nullptr;
  poolInfo.flags = 0; // Optional flag for allowing freeing after creation
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolDescriptors.size());
  poolInfo.pPoolSizes = poolDescriptors.data();
  // maximum number of sets that can be allocated
  poolInfo.maxSets =
      static_cast<uint32_t>(m_SwapImages.size()) * static_cast<uint32_t>(m_DescriptorLayouts.size());

  // Create the descriptor pool
  result = vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create descriptor pool");
  }
  return;
}

void GraphicsHandler::createDescriptorSets(void)
{
  for (const auto &image : m_SwapImages)
  {
    // Layout[0] = set0 -- Model + View bindings
    // Layout[1] = set1 -- Projection binding only
    m_Set0Allocs.push_back(m_DescriptorLayouts[0]);
    m_Set1Allocs.push_back(m_DescriptorLayouts[1]);
  }

  // Set0 -- Model + View -- Allocations
  VkDescriptorSetAllocateInfo set0AllocInfo{};
  set0AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set0AllocInfo.pNext = nullptr;
  set0AllocInfo.descriptorPool = m_DescriptorPool;
  set0AllocInfo.descriptorSetCount = static_cast<uint32_t>(m_Set0Allocs.size());
  set0AllocInfo.pSetLayouts = m_Set0Allocs.data();

  // Set1 -- Projection -- Allocations
  VkDescriptorSetAllocateInfo set1AllocInfo{};
  set1AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set1AllocInfo.pNext = nullptr;
  set1AllocInfo.descriptorPool = m_DescriptorPool;
  set1AllocInfo.descriptorSetCount = static_cast<uint32_t>(m_Set1Allocs.size());
  set1AllocInfo.pSetLayouts = m_Set1Allocs.data();

  // Set 0 Allocations
  m_ModelViewSets.resize(m_Set0Allocs.size());
  // Allocate sets
  if (vkAllocateDescriptorSets(m_Device, &set0AllocInfo, m_ModelViewSets.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to allocate model & view descriptor sets");
  }

  // Set 1 Allocations
  m_ProjectionSets.resize(m_Set1Allocs.size());
  if (vkAllocateDescriptorSets(m_Device, &set1AllocInfo, m_ProjectionSets.data()) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to allocate projection descriptor sets");
  }
  return;
}

void GraphicsHandler::bindDescriptorSets(void)
{
  for (const auto &image : m_SwapImages)
  {
  }
  return;
}

void GraphicsHandler::bindescriptorsets(void)
{
  for (size_t i = 0; i < m_SwapImages.size(); i++)
  {
    // Each descriptor info needs it's own descriptor set
    // VkDescriptorBufferInfo bufferInfo{};
    // bufferInfo.buffer = m_UniformBuffers[i];
    // bufferInfo.offset = 0;
    // bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorBufferInfo uniformModelBufferInfo{};
    uniformModelBufferInfo.buffer = m_UniformModelBuffers[i];
    uniformModelBufferInfo.offset = 0;
    uniformModelBufferInfo.range = sizeof(UniformModelBuffer);

    VkDescriptorBufferInfo uniformVPBufferInfo{};
    uniformVPBufferInfo.buffer = m_UniformVPBuffers[i];
    uniformVPBufferInfo.offset = 0;
    uniformVPBufferInfo.range = sizeof(UniformVPBuffer);

    // Descriptor writes container
    std::array<VkWriteDescriptorSet, 2> descriptorSets{};

    // Describe DescriptorWrites for uniform buffer object
    descriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSets[0].dstSet = m_DescriptorSets[i];
    descriptorSets[0].dstBinding = 0;
    descriptorSets[0].dstArrayElement = 0;
    descriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSets[0].descriptorCount = 1;
    descriptorSets[0].pBufferInfo = &uniformModelBufferInfo;
    descriptorSets[0].pImageInfo = nullptr;
    descriptorSets[0].pTexelBufferView = nullptr;

    descriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSets[1].dstSet = m_DescriptorSets[i];
    descriptorSets[1].dstBinding = 1;
    descriptorSets[1].dstArrayElement = 0;
    descriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSets[1].descriptorCount = 1;
    descriptorSets[1].pBufferInfo = &uniformVPBufferInfo;
    descriptorSets[1].pImageInfo = nullptr;
    descriptorSets[1].pTexelBufferView = nullptr;

    // Update the descriptor set specified in writeInfo
    vkUpdateDescriptorSets(m_Device,
                           static_cast<uint32_t>(descriptorSets.size()),
                           descriptorSets.data(),
                           0,
                           nullptr);
  }

  return;
}

// void GraphicsHandler::createIndexBuffer(void)
// {
//   // Index buffer size
//   VkDeviceSize bufferSize = INDEX_BUFFER_SIZE;

//   // Create the gpu local buffer and copy our staging buffer to it
//   createBuffer(
//       bufferSize,
//       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
//       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//       m_IndexBuffer,
//       m_IndexMemory);

//   return;
// }

// void GraphicsHandler::createVertexBuffer(void)
// {
//   VkDeviceSize bufferSize = VERTEX_BUFFER_SIZE;

//   // Create buffer device local
//   createBuffer(
//       bufferSize,
//       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//       m_VertexBuffer,
//       m_VertexMemory);
//   return;
// }

// void GraphicsHandler::createUniformBuffers(void)
// {
//   m_UniformBuffers.resize(m_SwapImages.size());
//   m_UniformMemory.resize(m_SwapImages.size());
//   m_UniformPtrs.resize(m_SwapImages.size());

//   for (const auto &image : m_SwapImages)
//   {
//     createBuffer(selectedDevice->devProperties.properties.limits.maxUniformBufferRange,
//                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
//                  m_UniformBuffers[&image - &m_SwapImages[0]],
//                  m_UniformMemory[&image - &m_SwapImages[0]]);

//     if (vkMapMemory(m_Device,
//                     m_UniformMemory[&image - &m_SwapImages[0]],
//                     0,
//                     VK_WHOLE_SIZE,
//                     0,
//                     &m_UniformPtrs[&image - &m_SwapImages[0]]) != VK_SUCCESS)
//     {
//       G_EXCEPT("Failed to map uniform buffer");
//     }
//   }
//   return;
// }

bool GraphicsHandler::loadDebugUtils(void)
{
  PFN_vkVoidFunction temp_fp;

  // load vkCreateDebugUtilsMessengerEXT
  temp_fp = vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
  if (!temp_fp)
  {
    G_EXCEPT("Failure loading vkCreateDebugUtilsMessengerEXT");
  }
  vkCreateDebugUtilsMessengerEXT =
      reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(temp_fp);

  // load vkDestroyDebugUtilsMessengerEXT
  temp_fp =
      vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
  if (!temp_fp)
  {
    G_EXCEPT("Failure loading vkDestroyDebugUtilsMessengerEXT");
  }

  vkDestroyDebugUtilsMessengerEXT =
      reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(temp_fp);

  // load vkSubmitDebugUtilsMessageEXT
  temp_fp = vkGetInstanceProcAddr(m_Instance, "vkSubmitDebugUtilsMessageEXT");
  if (!temp_fp)
  {
    G_EXCEPT("Failure loading vkSubmitDebugUtilsMessageEXT");
  }

  vkSubmitDebugUtilsMessageEXT =
      reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(temp_fp);

  return true;
}

VkShaderModule GraphicsHandler::createShaderModule(const std::vector<char> &code)
{
  VkShaderModule module;

  VkShaderModuleCreateInfo moduleInfo{};
  moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleInfo.pNext = nullptr;
  moduleInfo.flags = 0;
  moduleInfo.codeSize = code.size();
  moduleInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(m_Device, &moduleInfo, nullptr, &module) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create shader module!");
  }

  return module;
}

VKAPI_ATTR
VkBool32
    VKAPI_CALL
    debugMessageProcessor(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        __attribute__((unused)) VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
        __attribute__((unused)) void *user_data)
{
  if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    std::ostringstream oss;
    oss << std::endl
        << "Warning: " << callback_data->messageIdNumber
        << ", " << callback_data->pMessageIdName << std::endl
        << callback_data->pMessage << std::endl
        << std::endl;
    std::cout << oss.str();
  }
  else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT && 1 == 2)
  {
    // Disabled by the impossible statement
    std::ostringstream oss;
    oss << std::endl
        << "Verbose message : " << callback_data->messageIdNumber << ", " << callback_data->pMessageIdName
        << std::endl
        << callback_data->pMessage << std::endl
        << std::endl;
    std::cout << oss.str();
  }
  else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    std::ostringstream oss;
    oss << std::endl
        << "Error: " << callback_data->messageIdNumber
        << ", " << callback_data->pMessageIdName << std::endl
        << callback_data->pMessage << std::endl
        << std::endl;
    std::cout << oss.str();
  }
  else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    std::ostringstream oss;
    oss << std::endl
        << "Info: " << callback_data->messageIdNumber
        << ", " << callback_data->pMessageIdName << std::endl
        << callback_data->pMessage << std::endl
        << std::endl;
  }
  return VK_FALSE;
}

/*
  Iterates through supported formats found when querying the swap chain
  Selects VK_FORMAT_B8G8R8A8_SRGB if it exists as it is one of the most common formats for images
  and non linear sRGB as it is also the most common
*/
VkSurfaceFormatKHR GraphicsHandler::chooseSwapChainFormat(void)
{
  for (const auto &availableFormat : m_SurfaceDetails.formats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return m_SurfaceDetails.formats[0];
}

// Returns
VkPresentModeKHR GraphicsHandler::chooseSwapChainPresentMode(void)
{
  for (const auto &presentMode : m_SurfaceDetails.presentModes)
  {
    if (presentMode == PRESENT_MODE)
    {
      return PRESENT_MODE;
    }
  }

  // FIFO guaranteed to be available
  std::cout << "[+] Present mode -> " << presentModeToString(PRESENT_MODE)
            << " <- was not available, defaulting to "
            << presentModeToString(VK_PRESENT_MODE_FIFO_KHR) << std::endl;
  return VK_PRESENT_MODE_FIFO_KHR;
}

/*
  Set resolution of swap chain images
  should be equal to render area of window
*/
VkExtent2D GraphicsHandler::chooseSwapChainExtent(void)
{
  if (m_SurfaceDetails.capabilities.currentExtent.width != UINT32_MAX)
  {
    return m_SurfaceDetails.capabilities.currentExtent;
  }
  else
  {
    Window rw;
    int rx, ry;           // returned x/y
    uint rwidth, rheight; // returned width/height
    uint rborder;         // returned  border width
    uint rdepth;          // returned bit depth
    if (!XGetGeometry(display,
                      *window, &rw, &rx, &ry,
                      &rwidth, &rheight, &rborder, &rdepth))
    {
      G_EXCEPT("[-] Failed to get window geometry!");
    }

    VkExtent2D actualExtent;
    actualExtent.width = static_cast<uint32_t>(rwidth);
    actualExtent.height = static_cast<uint32_t>(rheight);

    // do some min max clamping
    actualExtent.width = std::max(
        m_SurfaceDetails.capabilities.minImageExtent.width,
        std::min(
            m_SurfaceDetails.capabilities.maxImageExtent.width,
            actualExtent.width));

    actualExtent.height = std::max(
        m_SurfaceDetails.capabilities.minImageExtent.height,
        std::min(m_SurfaceDetails.capabilities.maxImageExtent.height,
                 actualExtent.height));
    return actualExtent;
  }
}

std::vector<char> GraphicsHandler::readFile(std::string filename)
{
  size_t fileSize;
  std::ifstream file;
  std::vector<char> buffer;

  // check if file exists
  try
  {
    std::filesystem::exists(filename);
    file.open(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
      std::ostringstream oss;
      oss << "Failed to open file " << filename;
      G_EXCEPT(oss.str());
    }

    // prepare buffer to hold shader bytecode
    fileSize = (size_t)file.tellg();
    buffer.resize(fileSize);

    // go back to beginning of file and read in
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
  }
  catch (std::filesystem::filesystem_error &e)
  {
    std::ostringstream oss;
    oss << "Filesystem Exception : " << e.what() << std::endl;
    G_EXCEPT(oss.str());
  }
  catch (std::exception &e)
  {
    std::ostringstream oss;
    oss << "Standard Exception : " << e.what();
    G_EXCEPT(oss.str());
  }
  catch (...)
  {
    G_EXCEPT("Unhandled exception while loading a file");
  }

  if (buffer.empty())
  {
    G_EXCEPT("File reading operation returned empty buffer");
  }

  return buffer;
}

void GraphicsHandler::recreateSwapChain(void)
{
  vkDeviceWaitIdle(m_Device);

  /*
    Ensure resources weren't previously destroyed before
    attempting to do so again
  */
  if (SHOULD_RENDER)
  {
    // free up binded resources for recreation
    cleanupSwapChain();
  }

  // Get new details
  std::cout << "[/] Creating new swap chain" << std::endl;
  querySwapChainSupport();

  if (m_SurfaceDetails.formats.empty() ||
      m_SurfaceDetails.presentModes.empty())
  {
    G_EXCEPT("Selected device does not support any presents/formats");
  }

  // if any dimensions are 0, skip resource creation
  if (m_SurfaceDetails.capabilities.currentExtent.width == 0 ||
      m_SurfaceDetails.capabilities.currentExtent.height == 0)
  {
    SHOULD_RENDER = false;
    return;
  }
  else
  {
    SHOULD_RENDER = true;
  }

  // Creates swapchain and image views
  createSwapChain();

  createGraphicsPipeline();

  createFrameBuffers();

  createDescriptorPool();

  createDescriptorSets();

  createCommandBuffers();

  // Recreate the camera
  camera = std::make_unique<Camera>(m_SurfaceDetails.capabilities.currentExtent.width,
                                    m_SurfaceDetails.capabilities.currentExtent.height);

  std::cout << "\t[+] Done!" << std::endl;
  return;
}

// uint32_t GraphicsHandler::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
// {
//   VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
//   vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice,
//                                       &deviceMemoryProperties);

//   for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++)
//   {
//     if (typeFilter & (1 << i) &&
//         (deviceMemoryProperties.memoryTypes[i].propertyFlags &
//          properties) == properties)
//     {
//       return i;
//     }
//   }

//   G_EXCEPT("Failed to find suitable VRAM type");
// }

// void GraphicsHandler::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
// {
//   VkBufferCreateInfo bufferInfo{};
//   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//   bufferInfo.pNext = nullptr;
//   bufferInfo.flags = 0;
//   bufferInfo.size = size;
//   bufferInfo.usage = usage;
//   bufferInfo.sharingMode = m_SurfaceDetails.sharingMode;
//   if (m_SurfaceDetails.sharingMode == VK_SHARING_MODE_EXCLUSIVE)
//   {
//     bufferInfo.queueFamilyIndexCount = 1;
//     bufferInfo.pQueueFamilyIndices = &selectedDevice->graphicsFamilyIndex;
//   }
//   else if (m_SurfaceDetails.sharingMode == VK_SHARING_MODE_CONCURRENT)
//   {
//     uint32_t indices[] = {selectedDevice->graphicsFamilyIndex, selectedDevice->presentIndexes[0]};
//     bufferInfo.queueFamilyIndexCount = 2;
//     bufferInfo.pQueueFamilyIndices = indices;
//   }

//   if (vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
//   {
//     G_EXCEPT("Failed to create vertex buffer!");
//   }

//   // allocate memory for buffer
//   VkMemoryRequirements memRequirements{};
//   vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

//   // Describe memory allocation
//   VkMemoryAllocateInfo allocInfo{};
//   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//   allocInfo.pNext = nullptr;
//   allocInfo.allocationSize = memRequirements.size;
//   allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

//   if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
//   {
//     G_EXCEPT("Failed to allocate memory for vertex buffer!");
//   }

//   if (vkBindBufferMemory(m_Device, buffer, bufferMemory, 0) != VK_SUCCESS)
//   {
//     G_EXCEPT("Failed to bind memory to vertex buffer");
//   }
//   return;
// }

void GraphicsHandler::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, int dstOffset, VkDeviceSize size)
{
  // Create command buffer and prepare for recording
  // immediately ready to record after this function
  VkCommandBuffer commandBuffer = beginSingleCommands();

  // Describe copy command
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = dstOffset;
  copyRegion.size = size;

  // Record copy command
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  // end recording
  endSingleCommands(commandBuffer);

  return;
}

void GraphicsHandler::updateUniformModelBuffer(uint32_t imageIndex)
{
  // UniformModelBuffer um;

  // static auto startTime = std::chrono::high_resolution_clock::now();

  // auto currentTime = std::chrono::high_resolution_clock::now();
  // float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  // Human.humans.at(0).worldMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  // um.model = Human.humans.at(0).worldMatrix;
  // // ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  // // ubo.proj = glm::perspective(glm::radians(45.0f), selectedSwapExtent.width / (float)selectedSwapExtent.height, 0.1f, 10.0f);

  // // Flip due to legacy opengl matrix inversion
  // // should look into seeing whether this operation is faster on gpu side
  // // or should it remain here as a cpu job
  // /*
  //   Flip projection matrix as such
  //   proj[1][1] *= -1;
  // */

  // // Transfer data
  // memcpy(m_UniformModelPtrs[imageIndex], &um, sizeof(UniformModelBuffer));
  return;
}

void GraphicsHandler::updateUniformVPBuffer(uint32_t imageIndex)
{
  // UniformVPBuffer uvp;
  // // Point left hand down to visualize the axis
  // // Origin is at top left
  // uvp.view = glm::lookAt(camera->getPosition(), camera->getPosition() + camera->DEFAULT_FORWARD_VECTOR, glm::vec3(0.0f, -1.0f, 0.0f));
  // uvp.proj = glm::perspective(glm::radians(45.0f), selectedSwapExtent.width / (float)selectedSwapExtent.height, 0.1f, 100.0f);
  // uvp.proj[1][1] *= -1;

  // // Transfer
  // memcpy(m_UniformVPPtrs[imageIndex], &uvp, sizeof(UniformVPBuffer));
  return;
}

VkCommandBuffer GraphicsHandler::beginSingleCommands(void)
{
  VkResult result;

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = m_CommandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  // Initialize command buffer for recording
  result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create and begin a command buffer");
  }

  return commandBuffer;
}

void GraphicsHandler::endSingleCommands(VkCommandBuffer commandBuffer)
{
  VkResult result;

  // End recording within' command buffer
  result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to end recording command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = nullptr;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Submit queue for processing
  result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to submit queue");
  }

  // Wait for the queue to finish
  result = vkQueueWaitIdle(m_GraphicsQueue);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Error occurred waiting for queue to finish");
  }

  // Free up the command buffer
  vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
  return;
}

VkImageView GraphicsHandler::createImageView(VkImage image, VkFormat format)
{
  VkImageView imageView;

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.pNext = nullptr;
  viewInfo.flags = 0;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.components = {
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY};
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create image view");
  }

  return imageView;
}

/*
  THIS FUNCTION MUST BE CALLED AND PASSED MODELCLASS OBJECTS IN THE SAME ORDER THEY
  ARE CALLED IN ModelClass->loadModelData()

  loadModelData() GENERATES OFFSETS FROM WHAT THEY ARE GIVEN

  Loads vertex data into the buffer
*/
// void GraphicsHandler::processModelData(ModelClass *modelObj)
// {
//   std::cout << "[+] Moving allocated model :: " << modelObj->typeName
//             << " :: into gpu memory" << std::endl;

//   VkResult result;

//   VkBuffer vertexStagingBuffer{};
//   VkDeviceMemory vertexStagingMemory{};

//   VkBuffer indexStagingBuffer{};
//   VkDeviceMemory indexStagingMemory{};

//   // Create the vertex staging buffer
//   createBuffer(modelObj->vertexDataSize,
//                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                vertexStagingBuffer,
//                vertexStagingMemory);

//   // Map vertex staging
//   void *vertexDataPtr;
//   result = vkMapMemory(m_Device,
//                        vertexStagingMemory,
//                        0,
//                        modelObj->vertexDataSize,
//                        0,
//                        &vertexDataPtr);
//   if (result != VK_SUCCESS)
//   {
//     G_EXCEPT("Failed to map memory for processing of model data");
//   }
//   // fill vertex stage
//   memcpy(vertexDataPtr, modelObj->vertices.data(), modelObj->vertexDataSize);

//   // Unmap vertex staging
//   vkUnmapMemory(m_Device, vertexStagingMemory);

//   // Copy vertex stage -> vertex buffer
//   copyBuffer(vertexStagingBuffer, m_VertexBuffer, modelObj->vertexStartOffset, modelObj->vertexDataSize);
//   std::cout << "Copying vertex data into buffer at dst offset -> " << modelObj->vertexStartOffset << std::endl;

//   // create index stage
//   createBuffer(modelObj->indexDataSize,
//                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                indexStagingBuffer,
//                indexStagingMemory);

//   // Map index stage
//   void *indexDataPtr;
//   result = vkMapMemory(m_Device,
//                        indexStagingMemory,
//                        0,
//                        modelObj->indexDataSize,
//                        0,
//                        &indexDataPtr);
//   if (result != VK_SUCCESS)
//   {
//     G_EXCEPT("Failed to map memory for processing of model data");
//   }

//   // fill index stage
//   memcpy(indexDataPtr, modelObj->indices.data(), modelObj->indexDataSize);

//   // unmap index stage
//   vkUnmapMemory(m_Device, indexStagingMemory);

//   // Copy index stage -> index buffer
//   copyBuffer(indexStagingBuffer, m_IndexBuffer, modelObj->indexStartOffset, modelObj->indexDataSize);
//   std::cout << "Copying index data into buffer at dst offset -> " << modelObj->indexStartOffset << std::endl;

//   // cleanup vertex stage
//   vkDestroyBuffer(m_Device, vertexStagingBuffer, nullptr);
//   vkFreeMemory(m_Device, vertexStagingMemory, nullptr);

//   // cleanup index stage
//   vkDestroyBuffer(m_Device, indexStagingBuffer, nullptr);
//   vkFreeMemory(m_Device, indexStagingMemory, nullptr);
//   return;
// }

/*
  The index buffer and vertex buffer have been created

  This will load...
  vertex data into the vertex buffer AND
  index data into the index buffer AND
  MVP matrice data into an MVP buffer

  Vertex and Index data are device local
  MVP matrice data buffer is HOST_VISIBLE and can simply be written to each frame
*/
// void GraphicsHandler::loadEntities(void)
// {
//   std::pair<int, int> prevBufferOffsets = {0, 0};
//   std::pair<int, int> nextBufferOffsets = {0, 0};

//   std::cout << "\tModel -> " << Human.typeName << std::endl;
//   nextBufferOffsets = Human.loadModelData(prevBufferOffsets);
//   std::cout << "Model Previous -> " << prevBufferOffsets.first << ", " << prevBufferOffsets.second << std::endl;
//   std::cout << "Model next -> " << nextBufferOffsets.first << ", " << nextBufferOffsets.second << std::endl;

//   if (nextBufferOffsets.first == -1 ||
//       nextBufferOffsets.second == -1)
//   {
//     G_EXCEPT("Model data would exceed allocated buffer limits!");
//   }

//   // Update offset
//   prevBufferOffsets = nextBufferOffsets;

//   // Load grid vertices
//   nextBufferOffsets = loadGridVertices(prevBufferOffsets);
//   std::cout << "Grid Previous -> " << prevBufferOffsets.first << ", " << prevBufferOffsets.second << std::endl;
//   std::cout << "Grid next -> " << nextBufferOffsets.first << ", " << nextBufferOffsets.second << std::endl;

//   if (nextBufferOffsets.first == -1 ||
//       nextBufferOffsets.second == -1)
//   {
//     G_EXCEPT("Grid vertices would exceed buffer limits!");
//   }

//   // update offset
//   prevBufferOffsets = nextBufferOffsets;

//   std::cout << "After grid -> " << prevBufferOffsets.first << ", " << prevBufferOffsets.second << std::endl;
//   std::cout << "After grid -> " << nextBufferOffsets.first << ", " << nextBufferOffsets.second << std::endl;

//   // After all model data is loaded we will pass those to an actual buffer
//   processModelData(&Human);
//   processGridData();
//   return;
// }

void GraphicsHandler::createGridVertices(void)
{
  // X units from origin in length
  // From origin to X && From origin to -X
  int gridLength = 30;

  // Grid is flat against the X & Z PLANE
  for (int i = -gridLength; i < gridLength; i++)
  {
    // Rows
    grid.push_back(Vertex({{-gridLength, 0.0f, i, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}));
    grid.push_back(Vertex({{gridLength, 0.0f, i, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}));

    // Columns
    grid.push_back(Vertex({{i, 0.0f, -gridLength, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}})); // negative z - blue
    grid.push_back(Vertex({{i, 0.0f, gridLength, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}));  // positive z - green
  }

  // Calculate vertices size
  gridVertexDataSize = sizeof(Vertex) * grid.size();

  // These vertices are loaded into a buffer by a separate function
  // That function will be passed offsets so as maintain integrity of vertex buffer
  return;
}

std::pair<int, int> GraphicsHandler::loadGridVertices(std::pair<int, int> previousOffsets)
{
  gridStartOffset = previousOffsets.first;
  int endVertexOffset = previousOffsets.first + gridVertexDataSize;

  if (endVertexOffset > VERTEX_BUFFER_SIZE)
  {
    std::cout << "\t[-] Self check : Grid data will exceed vertex buffer size" << std::endl;
    return {-1, -1};
  }

  return {endVertexOffset, previousOffsets.second};
}

void GraphicsHandler::processGridData(void)
{
  std::cout << "[+] Loading grid vertices into buffer" << std::endl;

  VkResult result;

  VkBuffer vertexStagingBuffer{};
  VkDeviceMemory vertexStagingMemory{};

  // Create the vertex staging buffer
  createBuffer(gridVertexDataSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               vertexStagingBuffer,
               vertexStagingMemory);

  // Map vertex staging
  void *vertexDataPtr;
  result = vkMapMemory(m_Device,
                       vertexStagingMemory,
                       0,
                       gridVertexDataSize,
                       0,
                       &vertexDataPtr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to map memory for processing of grid data");
  }
  // fill vertex stage
  memcpy(vertexDataPtr, grid.data(), gridVertexDataSize);

  // Unmap vertex staging
  vkUnmapMemory(m_Device, vertexStagingMemory);

  // Copy vertex stage -> vertex buffer
  copyBuffer(vertexStagingBuffer, m_VertexBuffer, gridStartOffset, gridVertexDataSize);
  std::cout << "Copying grid data into buffer at dst offset -> " << gridStartOffset << std::endl;

  // Cleanup
  vkDestroyBuffer(m_Device, vertexStagingBuffer, nullptr);
  vkFreeMemory(m_Device, vertexStagingMemory, nullptr);
  return;
}

void GraphicsHandler::recordCommandBuffer(uint32_t imageIndex)
{
  VkResult result;

  // Begin recording command buffer
  UniformModelBuffer identityMatrix;
  identityMatrix.model = glm::mat4(1.0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.pNext = nullptr;
  beginInfo.flags = 0;
  beginInfo.pInheritanceInfo = nullptr;

  result = vkBeginCommandBuffer(m_CommandBuffers[imageIndex], &beginInfo);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to begin command buffer!");
  }

  // Begin render pass
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.pNext = nullptr;
  renderPassInfo.renderPass = m_RenderPass;
  renderPassInfo.framebuffer = m_Framebuffers[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = selectedSwapExtent;

  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(m_CommandBuffers[imageIndex],
                       &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(m_CommandBuffers[imageIndex],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_Pipeline);

  // Due to using dynamic state, primitive topology must be set
  // render pass
  vkCmdSetPrimitiveTopologyEXT(m_CommandBuffers[imageIndex], VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

  // Bind index buffer here
  vkCmdBindIndexBuffer(m_CommandBuffers[imageIndex],
                       m_IndexBuffer,
                       0,
                       VK_INDEX_TYPE_UINT16);
  // Bind vertex buffers
  VkBuffer vertexBuffers[] = {m_VertexBuffer};
  std::vector<VkDeviceSize> offsets = {Human.vertexStartOffset};
  vkCmdBindVertexBuffers(m_CommandBuffers[imageIndex], 0, 1, vertexBuffers, offsets.data());

  // Bind our descriptor sets
  std::vector<uint32_t> dynamicOffsets = {0, 0};
  vkCmdBindDescriptorSets(m_CommandBuffers[imageIndex],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_PipelineLayout,
                          0,
                          1,
                          &m_DescriptorSets[imageIndex],
                          2,
                          dynamicOffsets.data());

  // Iterate all objects and draw
  // Later optimisation will determine which SHOULD be rendered and not

  // Draw human entities
  // vkCmdDraw(m_CommandBuffers[i], static_cast<uint32_t>(Human.vertices.size()), 1, 0, 0);
  identityMatrix.model = Human.humans.at(0).worldMatrix;
  vkCmdPushConstants(m_CommandBuffers[imageIndex], m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformModelBuffer), &identityMatrix);
  vkCmdDrawIndexed(m_CommandBuffers[imageIndex],
                   static_cast<uint32_t>(Human.indices.size()),
                   1,
                   Human.indexStartOffset,
                   Human.vertexStartOffset,
                   0);

  /*
      Rebind vertex buffer at new offset for grid data
    */
  offsets = {gridStartOffset};
  vkCmdBindVertexBuffers(m_CommandBuffers[imageIndex], 0, 1, vertexBuffers, offsets.data());

  identityMatrix.model = glm::mat4(1.0);
  vkCmdPushConstants(m_CommandBuffers[imageIndex], m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformModelBuffer), &identityMatrix);
  vkCmdDraw(m_CommandBuffers[imageIndex], grid.size(), 1, 0, 0);

  vkCmdEndRenderPass(m_CommandBuffers[imageIndex]);

  result = vkEndCommandBuffer(m_CommandBuffers[imageIndex]);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Error ending command buffer recording");
  }
  return;
}

/* CLEANUP / DESTRUCTOR */

GraphicsHandler::~GraphicsHandler()
{
  std::cout << "[+] Cleaning up Vulkan resources" << std::endl;

  // Wait for all queues to complete before destruction
  vkDeviceWaitIdle(m_Device);

  cleanupSwapChain();

  // Cleanup descriptor layouts
  if (!m_DescriptorLayouts.empty())
  {
    for (auto &layout : m_DescriptorLayouts)
    {
      vkDestroyDescriptorSetLayout(m_Device, layout, nullptr);
    }
  }

  // /* Vertex buffer/memory */
  // if (m_VertexBuffer != VK_NULL_HANDLE)
  // {
  //   vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
  // }
  // if (m_VertexMemory != VK_NULL_HANDLE)
  // {
  //   vkFreeMemory(m_Device, m_VertexMemory, nullptr);
  // }

  // /* Index buffer/memory */
  // if (m_IndexBuffer != VK_NULL_HANDLE)
  // {
  //   vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
  // }
  // if (m_VertexMemory != VK_NULL_HANDLE)
  // {
  //   vkFreeMemory(m_Device, m_IndexMemory, nullptr);
  // }

  // /* Uniform buffers */

  // // UNMAP all MODEL MEMORIES
  // if (!m_UniformModelMemories.empty())
  // {
  //   for (auto &memory : m_UniformModelMemories)
  //   {
  //     vkUnmapMemory(m_Device, memory);
  //   }
  // }

  // // UNMAP all V/P MEMORIES
  // if (!m_UniformVPMemories.empty())
  // {
  //   for (auto &memory : m_UniformVPMemories)
  //   {
  //     vkUnmapMemory(m_Device, memory);
  //   }
  // }

  // // Destroy all MODEL BUFFERS
  // if (!m_UniformModelBuffers.empty())
  // {
  //   for (auto &buffer : m_UniformModelBuffers)
  //   {
  //     if (buffer != VK_NULL_HANDLE)
  //     {
  //       vkDestroyBuffer(m_Device, buffer, nullptr);
  //     }
  //   }
  // }

  // // Free all MODEL MEMORIES
  // if (!m_UniformModelMemories.empty())
  // {
  //   for (auto &memory : m_UniformModelMemories)
  //   {
  //     if (memory != VK_NULL_HANDLE)
  //     {
  //       vkFreeMemory(m_Device, memory, nullptr);
  //     }
  //   }
  // }

  // // Destroy all V/P BUFFERS
  // if (!m_UniformVPBuffers.empty())
  // {
  //   for (auto &buffer : m_UniformVPBuffers)
  //   {
  //     if (buffer != VK_NULL_HANDLE)
  //     {
  //       vkDestroyBuffer(m_Device, buffer, nullptr);
  //     }
  //   }
  // }

  // // Free all V/P MEMORIES
  // if (!m_UniformVPMemories.empty())
  // {
  //   for (auto &memory : m_UniformVPMemories)
  //   {
  //     if (memory != VK_NULL_HANDLE)
  //     {
  //       vkFreeMemory(m_Device, memory, nullptr);
  //     }
  //   }
  // }

  // Destroy m_imageAvailableSemaphore objects
  for (const auto &semaphore : m_imageAvailableSemaphore)
  {
    if (semaphore != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(m_Device, semaphore, nullptr);
    }
  }
  // Destroy m_renderFinishedSemaphore objects
  for (const auto &semaphore : m_renderFinishedSemaphore)
  {
    vkDestroySemaphore(m_Device, semaphore, nullptr);
  }
  // Destroy fence objects
  for (const auto &fence : m_inFlightFences)
  {
    if (fence != VK_NULL_HANDLE)
    {
      vkDestroyFence(m_Device, fence, nullptr);
    }
  }
  // Destroy command pool
  if (m_CommandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
  }
  if (m_Surface != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
  }
  if (m_Device != VK_NULL_HANDLE)
  {
    vkDestroyDevice(m_Device, nullptr);
  }
  if (m_Debug != VK_NULL_HANDLE)
  {
    vkDestroyDebugUtilsMessengerEXT(m_Instance, m_Debug, nullptr);
  }
  if (m_Instance != VK_NULL_HANDLE)
  {
    vkDestroyInstance(m_Instance, nullptr);
  }

  std::cout << "\t[+] Vulkan cleaned up!" << std::endl;
  return;
}

void GraphicsHandler::cleanupSwapChain(void)
{
  /*
    Reset camera object
    It relies on swapchain extent data
  */
  camera.reset();

  // Frame buffers
  for (const auto &buffer : m_Framebuffers)
  {
    if (buffer != VK_NULL_HANDLE)
    {
      vkDestroyFramebuffer(m_Device, buffer, nullptr);
    }
  }

  // Command buffers
  vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

  // Destroy pipeline object
  if (m_Pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
  }
  // Destroy pipeline layout
  if (m_PipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
  }
  // Destroy render pass object
  if (m_RenderPass != VK_NULL_HANDLE)
  {
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
  }
  // Free descriptor pool
  if (m_DescriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
  }
  // ensure we destroy all views to swap chain images
  if (!m_SwapViews.empty())
  {
    for (const auto &view : m_SwapViews)
    {
      if (view != VK_NULL_HANDLE)
      {
        vkDestroyImageView(m_Device, view, nullptr);
      }
    }
  }
  // Ensure to destroy images
  if (!m_SwapImages.empty())
  {
    for (auto &image : m_SwapImages)
    {
      vkDestroyImage(m_Device, image, nullptr);
    }
  }
  if (m_Swap != VK_NULL_HANDLE && m_Swap != nullptr)
  {
    vkDestroySwapchainKHR(m_Device, m_Swap, nullptr);
  }
  return;
}