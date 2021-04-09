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
  // Ensure all requested validation layers are supported
  if (enableValidationLayers)
  {
    std::string failList;
    std::cout << "[!] Debugging enabled" << std::endl;
    std::cout << "\t[-] Checking for validation layers" << std::endl;
    // Returns false if fail, failList is populated with failed requests
    bool validationCheck = checkValidationLayerSupport(&failList);
    if (!validationCheck)
    {
      std::string e = "The following requested validation layers were not found\n";
      e += failList;
      G_EXCEPT(e.c_str());
    }
    std::cout << "\t[+] All layers found" << std::endl;
  }

  // Ensure system supports requested instance extensions here
  std::string failList;
  std::cout << "[+] Checking instance level extension support" << std::endl;
  bool extensionCheck = checkInstanceExtensionSupport(&failList);

  // check if extension check failed
  if (!extensionCheck)
  {
    // if it failed, fetch the list of failed requests and throw
    std::string e = "The following requested instance extensions were not supported!\n";
    e += failList;
    e += "\nThis can sometimes mean that Vulkan is not supported by your system\n";
    G_EXCEPT(e.c_str());
  }

  initVulkan();

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
  queryDevices();

  /*
    Selects an adapter we will create a logical device for
    in the process it also retrieves the index of graphics queue family
    and stores as member variable of deviceInfoList[selectedIndex]
  */
  std::cout << "[+] Selecting adapter" << std::endl;
  if (!selectAdapter())
  {
    G_EXCEPT("Failed to find a device with Vulkan support");
  }

  // Store physical device handle in m_PhysicalDevice
  m_PhysicalDevice = deviceInfoList.at(selectedIndex).devHandle;

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

  // Fetches list of queues that support presentation
  findPresentSupport();

  /*
    Creates QueueCreateInfo structs for Graphics
    queue and if necessary a Present queue
  */
  configureCommandQueues();

  /* Fetch swap chain info for creation */
  querySwapChainSupport();

  


  /* RESUME REFACTORING HERE */
// checkdeviceextensionsupport






  // check for DEVICE extensions support
  std::string failList;
  std::cout << "[+] Checking for device level extension support" << std::endl;
  if (!checkDeviceExtensionSupport(&failList))
  {
    std::string e =
        "The following requested device extensions were not supported!\n";
    e += failList;
    G_EXCEPT(e.c_str());
  }

  std::cout << "[+] Creating logical device and queues" << std::endl;
  createLogicalDeviceAndQueues();

  // Load our device level PFN functions
  std::cout << "[+] Loading dynamic state functions" << std::endl;
  loadDevicePFN();

  /*
    Creates swap chain, retreives images and then creates views for all of them
  */
  std::cout << "[+] Creating swapchain" << std::endl;
  createSwapChain();

  /*
    Create descriptor layout for Vulkan to understand how to
    access uniform buffer resources (e.g model view projection matrices)
  */
  std::cout << "[+] Creating descriptor layouts" << std::endl;
  createDescriptorSetLayout();

  /*
    We've created our swap chain and retrieved the handle to all allocated images
    We've also created views into all retrieved images and are prepared for the
    final steps in created our pipeline
  */
  std::cout << "[+] Initializing pipeline" << std::endl;
  createGraphicsPipeline();

  /*
    Create framebuffers for rendering to
  */
  std::cout << "[+] Obtaining frame buffers" << std::endl;
  createFrameBuffers();

  /*
    Command pools are the parent memory container from which command buffers are allocated from

    This function allocates memory for the graphics queue family
  */
  std::cout << "[+] Allocating for command pool" << std::endl;
  createCommandPool();

  /*
    Create's staging buffer and loads index data into  it
    uses vulkan transfer function to move into gpu side buffer

    Only allocates space, need to be called before createVertexBuffer()
  */
  std::cout << "[+] Allocating " << ((INDEX_BUFFER_SIZE / 1000) / 1000)
            << " MB for index buffer" << std::endl;
  createIndexBuffer();

  /*
    Creates a device local buffer of VERTEX_BUFFER_SIZE
  */
  std::cout << "[+] Allocating " << ((VERTEX_BUFFER_SIZE / 1000) / 1000)
            << " MB for vertex buffer" << std::endl;
  createVertexBuffer();

  /*
    Creates 20 MB uniform buffer that will contain model matrix data
    for every entity

    Creates another 20 MB uniform buffer for view/project matrix data
  */
  std::cout << "[+] Allocating " << (MAX_UNIFORM_BUFFER_SIZE / 1000)
            << " KB for model matrix buffer" << std::endl;
  std::cout << "[+] Allocating " << (MAX_UNIFORM_BUFFER_SIZE / 1000)
            << " KB for view/projection matrix buffer" << std::endl;
  createUniformBuffers();

#ifndef NDEBUG
  createGridVertices();
#endif

  /*
    Loads all defined models' data into vertex, index and MVP buffers
  */
  std::cout << "[+] Loading models..." << std::endl;
  loadEntities();

  // Temp
  // Add a single human element to type container
  Human.humans.push_back(HumanClass());

  /*
   Describes the constraints on allocation of descriptor sets
   type, number/size etc
  */
  createDescriptorPool();

  /*
    Allocates descriptor sets from the preallocated pool
  */
  createDescriptorSets();

  /*
    These are buffers that a specified queue family's commands are recorded in

    This allocates memory from a parent command pool
  */
  std::cout << "[+] Allocating for command buffer" << std::endl;
  createCommandBuffers();

  /*
    Used to synchronize operation

    So that commands are not executed when resources are not yet available
    For ex: pipeline images
  */
  std::cout << "[+] Creating synchronization resources" << std::endl;
  createSyncObjects();

  // Create the camera
  camera = std::make_unique<Camera>(selectedSwapExtent.width, selectedSwapExtent.height);

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

bool GraphicsHandler::selectAdapter(void)
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

  // Ensure non zero rating
  if (deviceInfoList.at(selectedIndex).rating <= 0)
  {
    return false;
  }

  selectedDevice = &deviceInfoList.at(selectedIndex);

  // set our uniform buffer max size
  MAX_UNIFORM_BUFFER_SIZE = selectedDevice->devProperties.properties.limits.maxUniformBufferRange;
  return true;
}

void GraphicsHandler::findPresentSupport(void)
{
  for (auto &queue : selectedDevice->queueFamiles)
  {
    VkBool32 canPresent = false;
    uint32_t currentIndex = (&queue - &deviceInfoList.at(selectedIndex).queueFamiles[0]);
    VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice,
                                                           currentIndex,
                                                           m_Surface,
                                                           &canPresent);
    if (result != VK_SUCCESS)
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
  // Double check we have present and graphics queue
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

  // See if graphics queue supports presentation
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

// Query device for swapchain support
// Configure
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
  if(m_SurfaceDetails.presentModes.empty())
  {
    G_EXCEPT("Selected device has no supported present modes");
  }
  return;
}

void GraphicsHandler::createLogicalDeviceAndQueues(void)
{
  VkResult result;

  // logical device creation needs to occur
  // after preparing creation of all queues
  // now that we've described all queues we want to create,
  // past into creation of logical device
  VkDeviceCreateInfo logicalDeviceCreateInfo{};
  logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  logicalDeviceCreateInfo.pNext = &deviceInfoList.at(selectedIndex).devFeatures;

  logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

  // logicalDeviceCreateInfo.pEnabledFeatures = &deviceInfoList.at(selectedIndex).devFeatures.features;
  logicalDeviceCreateInfo.pEnabledFeatures = nullptr; // if pNext is vkPhysicalDeviceFeatures2 then must be nullptr

  // device level layers deprecated
  logicalDeviceCreateInfo.enabledLayerCount = 0;
  logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;

  logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size());
  logicalDeviceCreateInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();

  // Create logical device
  result = vkCreateDevice(m_PhysicalDevice,
                          &logicalDeviceCreateInfo,
                          nullptr,
                          &m_Device);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create logical device!");
  }

  // Retrieve the Graphics queue from newly created logical device
  vkGetDeviceQueue(m_Device,
                   deviceInfoList.at(selectedIndex).graphicsFamilyIndex,
                   0,
                   &m_GraphicsQueue);

  // Check if the presentation/graphics queue share
  // an index before requesting both
  // if not request present queue seperately
  if (deviceInfoList.at(selectedIndex).graphicsFamilyIndex !=
      deviceInfoList.at(selectedIndex).presentFamilyIndex)
  {
    vkGetDeviceQueue(m_Device,
                     deviceInfoList.at(selectedIndex).presentFamilyIndex,
                     0,
                     &m_PresentQueue);
  }
  else
  {
    // since the indexes are the same we will point present queue to graphics
    m_PresentQueue = m_GraphicsQueue;
  }

  // Ensure we retrieved the queue
  if (!m_GraphicsQueue)
  {
    G_EXCEPT("Failed to retrieve handle to graphics command queue");
  }
  return;
}

void GraphicsHandler::createSwapChain(void)
{
  VkResult result;

  /*
    Choose surface format, present mode and extent

    swap extent and format are member variables stored for future use
    `selectedSwapExtent`
    `selectedSwapFormat`
  */
  VkPresentModeKHR swapPresentMode{};
  uint32_t imageCount = 0;

  selectedSwapFormat = chooseSwapChainFormat();
  swapPresentMode = chooseSwapChainPresentMode();
  selectedSwapExtent = chooseSwapChainExtent();
  imageCount = m_SurfaceDetails.capabilities.maxImageCount + 1;

  // double check we did not exceed max image count
  if (m_SurfaceDetails.capabilities.maxImageCount > 0 &&
      imageCount > m_SurfaceDetails.capabilities.maxImageCount)
  {
    imageCount = m_SurfaceDetails.capabilities.maxImageCount;
  }

  // Now we can describe the creation of our swapchain
  VkSwapchainCreateInfoKHR swapChainCreateInfo{};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.pNext = nullptr;
  swapChainCreateInfo.flags = 0;
  swapChainCreateInfo.surface = m_Surface;
  swapChainCreateInfo.minImageCount = imageCount;
  swapChainCreateInfo.imageFormat = selectedSwapFormat.format;
  swapChainCreateInfo.imageColorSpace = selectedSwapFormat.colorSpace;
  swapChainCreateInfo.imageExtent = selectedSwapExtent;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  /*
    If presentQueue index and graphicsQueue index are the same we set our swap sharing mode to
    exclusive as images are handled within' 1 queue

    if they are seperate queues then we must set to swap sharing to concurrent and define which queue indexes will
    be sharing images here
  */
  if (deviceInfoList.at(selectedIndex).presentFamilyIndex ==
      deviceInfoList.at(selectedIndex).graphicsFamilyIndex)
  {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  }
  else
  {
    uint32_t queueIndices[] = {
        deviceInfoList.at(selectedIndex).presentFamilyIndex,
        deviceInfoList.at(selectedIndex).graphicsFamilyIndex};
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2;
    swapChainCreateInfo.pQueueFamilyIndices = queueIndices;
  }
  swapChainCreateInfo.preTransform =
      m_SurfaceDetails.capabilities.currentTransform;
  swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainCreateInfo.presentMode = swapPresentMode;
  swapChainCreateInfo.clipped = VK_TRUE;
  swapChainCreateInfo.oldSwapchain = nullptr;

  // Create the swap chain
  result = vkCreateSwapchainKHR(m_Device,
                                &swapChainCreateInfo,
                                nullptr,
                                &m_Swap);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create swap chain!");
  }

  // retrieve swap chain images that were created
  // could be higher than our specified amount in `minImageCount`

  uint32_t swapImageCount = 0;

  // get count of images
  result = vkGetSwapchainImagesKHR(m_Device, m_Swap, &swapImageCount, nullptr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Error getting count of swap chain images");
  }

  // ensure non zero
  if (swapImageCount == 0)
  {
    G_EXCEPT("Swapchain did not create any images!");
  }

  // get handles of images for purposes of creating views for them
  m_SwapImages.resize(swapImageCount);
  result = vkGetSwapchainImagesKHR(m_Device,
                                   m_Swap,
                                   &swapImageCount,
                                   m_SwapImages.data());
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Error getting swap chain image handles!");
  }

  // resize views container to be the same as swap images container
  m_SwapViews.resize(swapImageCount);

  for (size_t i = 0; i < m_SwapImages.size(); i++)
  {
    m_SwapViews[i] = createImageView(m_SwapImages[i], selectedSwapFormat.format);
  }

  return;
}

void GraphicsHandler::createDescriptorSetLayout(void)
{
  VkResult result;

  // Model matrix uniform buffer
  VkDescriptorSetLayoutBinding uniformModelBinding{};
  uniformModelBinding.binding = 0;
  uniformModelBinding.descriptorCount = 1;
  uniformModelBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uniformModelBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uniformModelBinding.pImmutableSamplers = nullptr;

  // View/project matrix uniform buffer
  VkDescriptorSetLayoutBinding uniformVPBinding{};
  uniformVPBinding.binding = 1;
  uniformVPBinding.descriptorCount = 1;
  uniformVPBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uniformVPBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uniformVPBinding.pImmutableSamplers = nullptr;

  // Container for bindings pointed to in layout description
  std::array<VkDescriptorSetLayoutBinding, 2> allBindings = {uniformModelBinding, uniformVPBinding};

  // Describe creation of layout
  VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
  uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  uboLayoutInfo.pNext = nullptr;
  uboLayoutInfo.flags = 0;
  uboLayoutInfo.bindingCount = static_cast<uint32_t>(allBindings.size());
  uboLayoutInfo.pBindings = allBindings.data();

  result = vkCreateDescriptorSetLayout(m_Device, &uboLayoutInfo, nullptr, &m_DescriptorLayout);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create uniform buffer descriptor layout");
  }
  return;
}

void GraphicsHandler::createGraphicsPipeline(void)
{
  VkResult result;

  auto vertexShader = readFile("shaders/vert.spv");
  auto fragShader = readFile("shaders/frag.spv");

  // ensure buffers not empty
  if (vertexShader.empty())
  {
    G_EXCEPT("Failed to load vertex shader!");
  }
  if (vertexShader.empty())
  {
    G_EXCEPT("Failed to load fragment shader!");
  }

  /*
    Wrap shader buffers as modules

    Modules can and should be destroyed after creation of graphics pipeline
  */
  VkShaderModule vertexModule = createShaderModule(vertexShader);
  VkShaderModule fragModule = createShaderModule(fragShader);

  /* Assign shader modules to stages of pipeline */

  // describe vertex shader stage
  VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
  vertexShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderStageInfo.pNext = nullptr;
  vertexShaderStageInfo.flags = 0;
  vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStageInfo.module = vertexModule;
  vertexShaderStageInfo.pName = "main";
  vertexShaderStageInfo.pSpecializationInfo = nullptr;

  // describe fragment shader stage
  VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
  fragmentShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShaderStageInfo.pNext = nullptr;
  fragmentShaderStageInfo.flags = 0;
  fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStageInfo.module = fragModule;
  fragmentShaderStageInfo.pName = "main";
  fragmentShaderStageInfo.pSpecializationInfo = nullptr;

  // store creation descriptions in array
  VkPipelineShaderStageCreateInfo pipelineStages[] = {
      vertexShaderStageInfo, fragmentShaderStageInfo};

  // describe creation of vertex input stage
  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescription = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pNext = nullptr;
  vertexInputInfo.flags = 0;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescription.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

  /*
    IGNORED WHEN `VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT` IS SET
    and pointed to with pDynamicState in the pipeline create info struct
  */
  // describe the type of geometry vulkan will be drawing
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
  inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyInfo.pNext = nullptr;
  inputAssemblyInfo.flags = 0;
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  // configure viewport ( what portion of frame buffer to render to )
  VkViewport viewport{};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width =
      static_cast<float>(selectedSwapExtent.width);
  viewport.height =
      static_cast<float>(selectedSwapExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  // configure scissor ( what portion of framebuffer to send to output merger )
  // use entire frame buffer
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = selectedSwapExtent;

  // combine viewport and scissor into a vulkan viewport state
  VkPipelineViewportStateCreateInfo viewportStateInfo{};
  viewportStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateInfo.pNext = nullptr;
  viewportStateInfo.flags = 0;
  viewportStateInfo.viewportCount = 1;
  viewportStateInfo.pViewports = &viewport;
  viewportStateInfo.scissorCount = 1;
  viewportStateInfo.pScissors = &scissor;

  // configure the rasterizer
  VkPipelineRasterizationStateCreateInfo rasterInfo{};
  rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterInfo.pNext = nullptr;
  rasterInfo.flags = 0;
  rasterInfo.depthClampEnable = VK_FALSE;
  // true disables output to frame buffer
  rasterInfo.rasterizerDiscardEnable = VK_FALSE;
  // wireframe vs fill
  rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterInfo.depthBiasEnable = VK_FALSE;
  rasterInfo.depthBiasConstantFactor = 0.0f;
  rasterInfo.depthBiasClamp = 0.0f;
  rasterInfo.depthBiasSlopeFactor = 0.0f;
  rasterInfo.lineWidth = 1.0f;

  // configure multisampling
  VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
  multisamplingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingInfo.pNext = nullptr;
  multisamplingInfo.flags = 0;
  multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisamplingInfo.sampleShadingEnable = VK_FALSE;
  multisamplingInfo.minSampleShading = 1.0f;
  multisamplingInfo.pSampleMask = nullptr;
  multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
  multisamplingInfo.alphaToOneEnable = VK_FALSE;

  // configure depth/stencil testing
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
  depthStencilInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilInfo.pNext = nullptr;
  depthStencilInfo.flags = 0;
  depthStencilInfo.depthTestEnable = VK_TRUE;
  depthStencilInfo.depthWriteEnable = VK_FALSE;
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencilInfo.stencilTestEnable = VK_FALSE;
  depthStencilInfo.front = {};
  depthStencilInfo.back = {};
  depthStencilInfo.minDepthBounds = 0.0f;
  depthStencilInfo.maxDepthBounds = 1.0f;

  // per frame buffer configuration of color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo{};
  colorBlendAttachmentInfo.blendEnable = VK_TRUE;
  colorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachmentInfo.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentInfo.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT |
      VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;

  // global color blending constants
  VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
  colorBlendingInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingInfo.pNext = nullptr;
  colorBlendingInfo.flags = 0;
  colorBlendingInfo.logicOpEnable = VK_FALSE;
  colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendingInfo.attachmentCount = 1;
  colorBlendingInfo.pAttachments = &colorBlendAttachmentInfo;
  colorBlendingInfo.blendConstants[0] = 0.0f;
  colorBlendingInfo.blendConstants[1] = 0.0f;
  colorBlendingInfo.blendConstants[2] = 0.0f;
  colorBlendingInfo.blendConstants[3] = 0.0f;

  // Describe pipeline layout creation
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

  VkPushConstantRange pConstantRange;
  pConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pConstantRange.size = sizeof(UniformModelBuffer);
  pConstantRange.offset = 0;

  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pNext = nullptr;
  pipelineLayoutInfo.flags = 0;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &m_DescriptorLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pConstantRange;

  // Create pipeline layout
  result = vkCreatePipelineLayout(m_Device,
                                  &pipelineLayoutInfo,
                                  nullptr,
                                  &m_PipelineLayout);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create pipeline layout!");
  }

  // defines render pass behavior
  createRenderPass();

  // describe pipeline object creation
  VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
  std::vector<VkDynamicState> dStates = {VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT};
  dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateInfo.pNext = nullptr;
  dynamicStateInfo.flags = 0;
  dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dStates.size());
  dynamicStateInfo.pDynamicStates = dStates.data();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;
  pipelineInfo.flags = 0;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = pipelineStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;

  // Using dynamic state for input assembly!
  // Set to nullptr when doing so
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;

  pipelineInfo.pTessellationState = nullptr;
  pipelineInfo.pViewportState = &viewportStateInfo;
  pipelineInfo.pRasterizationState = &rasterInfo;
  pipelineInfo.pMultisampleState = &multisamplingInfo;
  pipelineInfo.pDepthStencilState = &depthStencilInfo;
  pipelineInfo.pColorBlendState = &colorBlendingInfo;

  // Don't forget dynamic set enabling!
  pipelineInfo.pDynamicState = &dynamicStateInfo;

  pipelineInfo.layout = m_PipelineLayout;
  pipelineInfo.renderPass = m_RenderPass;
  pipelineInfo.subpass = 0; // sub pass index
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  result = vkCreateGraphicsPipelines(m_Device,
                                     VK_NULL_HANDLE,
                                     1,
                                     &pipelineInfo,
                                     nullptr,
                                     &m_Pipeline);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create graphics pipeline object!");
  }

  vkDestroyShaderModule(m_Device, vertexModule, nullptr);
  vkDestroyShaderModule(m_Device, fragModule, nullptr);
  return;
}

void GraphicsHandler::createRenderPass(void)
{
  VkResult result;

  VkAttachmentDescription colorAttachment{};
  colorAttachment.flags = 0;
  colorAttachment.format = selectedSwapFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  /* Specify behavior of color/depth buffer before/after rendering */
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  /* Specify behavior of stencil buffer before/after rendering */
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  /* 
     Ignore swap chain image previous layout; After rendering automatically
     transistion to be ready for presentation
  */
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // configure pointer to subpass
  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.flags = 0;
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = nullptr;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pResolveAttachments = nullptr;
  subpass.pDepthStencilAttachment = nullptr;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  // describe creation of render pass
  VkRenderPassCreateInfo renderInfo{};
  renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderInfo.pNext = nullptr;
  renderInfo.flags = 0;
  renderInfo.attachmentCount = 1;
  renderInfo.pAttachments = &colorAttachment;
  renderInfo.subpassCount = 1;
  renderInfo.pSubpasses = &subpass;
  renderInfo.dependencyCount = 1;
  renderInfo.pDependencies = &dependency;

  // create render pass
  result = vkCreateRenderPass(m_Device, &renderInfo, nullptr, &m_RenderPass);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create render pass object!");
  }

  return;
}

void GraphicsHandler::createFrameBuffers(void)
{
  VkResult result;

  // resize framebuffers to have enough for each swapchain image views
  m_Framebuffers.resize(m_SwapViews.size());

  // iterate each swap chain image and create frame buffer for its
  for (size_t i = 0; i < m_SwapViews.size(); i++)
  {
    VkImageView attachments[] = {
        m_SwapViews[i]};
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.flags = 0;
    framebufferInfo.renderPass = m_RenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = selectedSwapExtent.width;
    framebufferInfo.height = selectedSwapExtent.height;
    framebufferInfo.layers = 1;

    result = vkCreateFramebuffer(m_Device,
                                 &framebufferInfo,
                                 nullptr,
                                 &m_Framebuffers[i]);
    if (result != VK_SUCCESS)
    {
      G_EXCEPT("Failed to create frame buffer for image view");
    }
  }

  return;
}

void GraphicsHandler::createCommandPool(void)
{
  VkResult result;

  VkCommandPoolCreateInfo commandPoolInfo{};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.pNext = nullptr;
  commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolInfo.queueFamilyIndex = deviceInfoList.at(selectedIndex).graphicsFamilyIndex;

  result = vkCreateCommandPool(m_Device,
                               &commandPoolInfo,
                               nullptr,
                               &m_CommandPool);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create graphics command pool");
  }

  return;
}

void GraphicsHandler::createVertexBuffer(void)
{
  VkDeviceSize bufferSize = VERTEX_BUFFER_SIZE;

  // Create buffer device local
  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_VertexBuffer,
      m_VertexMemory);
  return;
}

// Creates index buffer and loads with data via a staging buffer
void GraphicsHandler::createIndexBuffer(void)
{
  // Index buffer size
  VkDeviceSize bufferSize = INDEX_BUFFER_SIZE;

  // Create the gpu local buffer and copy our staging buffer to it
  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      m_IndexBuffer,
      m_IndexMemory);

  return;
}

void GraphicsHandler::createDescriptorPool(void)
{
  VkResult result;

  VkDescriptorPoolSize uniformModelPool{};
  uniformModelPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uniformModelPool.descriptorCount = static_cast<uint32_t>(m_SwapImages.size());

  VkDescriptorPoolSize uniformVPPool{};
  uniformVPPool.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  uniformVPPool.descriptorCount = static_cast<uint32_t>(m_SwapImages.size());

  // Container for the pool size structs
  std::array<VkDescriptorPoolSize, 2> poolDescriptors = {uniformModelPool, uniformVPPool};

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.pNext = nullptr;
  poolInfo.flags = 0; // Optional flag for allowing freeing after creation
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolDescriptors.size());
  poolInfo.pPoolSizes = poolDescriptors.data();
  // maximum number of sets that can be allocated
  poolInfo.maxSets = static_cast<uint32_t>(m_SwapImages.size());

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
  VkResult result;

  std::vector<VkDescriptorSetLayout>
      layouts(m_SwapImages.size(), m_DescriptorLayout);

  // Describe the allocation of descriptor sets
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.descriptorPool = m_DescriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapImages.size());
  allocInfo.pSetLayouts = layouts.data();

  // Resize our container for the sets
  m_DescriptorSets.resize(m_SwapImages.size());
  // Allocate sets
  result = vkAllocateDescriptorSets(m_Device, &allocInfo, m_DescriptorSets.data());
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to allocate descriptor sets");
  }

  // Populate the descriptor sets
  // This is where the uniform buffers are binded to the descriptor sets
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

// Searches for specified layers(Global variable validationLayers)
// and returns whether they were all available or not
bool GraphicsHandler::checkValidationLayerSupport(std::string *failList)
{
  uint32_t layerCount = 0;
  failList->clear();
  VkResult result;
  result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("There was an error getting instance layer count!");
  }

  // Ensure we found layers
  if (!layerCount)
  {
    *failList = "Layer check could not find any available layers on system!";
    return false;
  }

  std::vector<VkLayerProperties> availableLayers(layerCount);
  result = vkEnumerateInstanceLayerProperties(&layerCount,
                                              availableLayers.data());
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("There was an error getting instance layer list!");
  }

  // iterates through list of requested layers
  for (const char *layerName : requestedValidationLayers)
  {
    bool layerFound = false;

    // iterates through list of fetched supported layers
    for (const auto &fetchedLayer : availableLayers)
    {
      // check if we have a match between fetched layer and requested one
      if (strcmp(layerName, fetchedLayer.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    // if we made it through fetched list without finding layer
    // add to failure list
    if (!layerFound)
    {
      *failList += layerName;
      *failList += "\n";
    }
  }

  // if failList is not empty return errors
  // errors added to fail list as they occur so just return here
  if (!failList->empty())
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool GraphicsHandler::checkInstanceExtensionSupport(std::string *failList)
{
  uint32_t instanceExtensionCount = 0;
  VkResult result;
  failList->clear(); // ensure failList is clear

  // get the number of supported instance extensions
  result = vkEnumerateInstanceExtensionProperties(nullptr,
                                                  &instanceExtensionCount,
                                                  nullptr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("There was an error getting instance extension count!");
  }

  // ensure we found any extensions
  if (!instanceExtensionCount)
  {
    *failList =
        "Extension check could not find any available extensions on system";
    return false;
  }

  // fetch all the supported instances
  std::vector<VkExtensionProperties>
      availableInstanceExtensions(instanceExtensionCount);
  result =
      vkEnumerateInstanceExtensionProperties(
          nullptr,
          &instanceExtensionCount,
          availableInstanceExtensions.data());
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("There was an error getting instance extension list!");
  }

  // iterate requested INSTANCE extensions
  for (const auto &requestedExtensions : requestedInstanceExtensions)
  {
    bool extensionMatch = false;

    // iterate the list of all supported INSTANCE extensions
    for (const auto &fetchedExtensions : availableInstanceExtensions)
    {
      // check if match
      if (strcmp(requestedExtensions, fetchedExtensions.extensionName) == 0)
      {
        extensionMatch = true;
        break;
      }
    }

    // if failed to find match, add to failList
    if (!extensionMatch)
    {
      *failList += requestedExtensions;
      *failList += "\n";
    }
  }

  // if failList is not empty return failure
  if (!failList->empty())
  {
    return false;
  }

  return true;
}

// check for DEVICE level extensions
bool GraphicsHandler::checkDeviceExtensionSupport(std::string *failList)
{
  uint32_t deviceExtensionCount = 0;
  VkResult result;
  failList->clear(); // ensure failList is clear

  // get total number of supported DEVICE extensions
  result = vkEnumerateDeviceExtensionProperties(
      deviceInfoList.at(selectedIndex).devHandle,
      nullptr,
      &deviceExtensionCount,
      nullptr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("There was an error getting count of device extensions!");
  }

  std::vector<VkExtensionProperties>
      availableDeviceExtensions(deviceExtensionCount);

  // fetch list of all supported DEVICE extensions
  result = vkEnumerateDeviceExtensionProperties(
      deviceInfoList.at(selectedIndex).devHandle,
      nullptr,
      &deviceExtensionCount,
      availableDeviceExtensions.data());
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("There was an error getting device extension list!");
  }

  // iterate list of requested DEVICE extensions
  for (const auto &requestedExtension : requestedDeviceExtensions)
  {
    bool extensionMatch = false;
    // iterate list of supported DEVICE extensions
    for (const auto &fetchedDeviceExtension : availableDeviceExtensions)
    {
      if (strcmp(requestedExtension,
                 fetchedDeviceExtension.extensionName) == 0)
      {
        extensionMatch = true;
        break;
      }
    }

    // if failed to find requested DEVICE extension
    // add to failList
    if (!extensionMatch)
    {
      *failList += requestedExtension;
      *failList += "\n";
    }
  }

  return true;
}

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
  VkResult result;
  VkShaderModule module;

  VkShaderModuleCreateInfo moduleInfo{};
  moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleInfo.pNext = nullptr;
  moduleInfo.flags = 0;
  moduleInfo.codeSize = code.size();
  moduleInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  result = vkCreateShaderModule(m_Device, &moduleInfo, nullptr, &module);
  if (result != VK_SUCCESS)
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

VkPresentModeKHR GraphicsHandler::chooseSwapChainPresentMode(void)
{
#ifndef VSYNC_MODE
  return VK_PRESENT_MODE_IMMEDIATE_KHR;
#endif

  // application will prefer MAILBOX present mode as it is akin to
  // triple buffering with less latency
  for (const auto &presentMode : m_SurfaceDetails.presentModes)
  {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return presentMode;
    }
    else if (presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
    {
      return presentMode;
    }
  }

  // failsafe as it is guaranteed to be available
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
  if (!std::filesystem::exists(filename))
  {
    std::ostringstream oss;
    oss << "Specified file does not exist -> " << filename << std::endl;
    G_EXCEPT(oss.str());
  }

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
  m_SurfaceDetails = querySwapChainSupport();

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
  camera = std::make_unique<Camera>(selectedSwapExtent.width, selectedSwapExtent.height);

  std::cout << "\t[+] Done!" << std::endl;
  return;
}

uint32_t GraphicsHandler::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice,
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

  G_EXCEPT("Failed to find suitable VRAM type");
}

void GraphicsHandler::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
{
  VkResult result;

  // describe creation of vertex buffer
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.pNext = nullptr;
  bufferInfo.flags = 0;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferInfo.queueFamilyIndexCount = 0;
  bufferInfo.pQueueFamilyIndices = nullptr;

  // Create the buffer with given specs
  result = vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create vertex buffer!");
  }

  // allocate memory for buffer
  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

  // Describe memory allocation
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  // Allocate the memory
  result = vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to allocate memory for vertex buffer!");
  }

  // Now bind the memory to the buffer
  result = vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to bind memory to vertex buffer");
  }
  return;
}

void GraphicsHandler::createImage(uint32_t width,
                                  uint32_t height,
                                  VkFormat format,
                                  VkImageTiling tiling,
                                  VkImageUsageFlags usage,
                                  VkMemoryPropertyFlags properties,
                                  VkImage &image,
                                  VkDeviceMemory &imageMemory)
{
  VkResult result;

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.pNext = nullptr;
  imageInfo.flags = 0;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = format;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = tiling;
  imageInfo.usage = usage;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.queueFamilyIndexCount = 0;     // not used when exclusive
  imageInfo.pQueueFamilyIndices = nullptr; // not used when exclusive
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  result = vkCreateImage(m_Device, &imageInfo, nullptr, &image);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create image");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = nullptr;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  result = vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory);

  result = vkBindImageMemory(m_Device, image, imageMemory, 0);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to bind image memory");
  }
  return;
}

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
  UniformModelBuffer um;

  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  Human.humans.at(0).worldMatrix = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  um.model = Human.humans.at(0).worldMatrix;
  // ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  // ubo.proj = glm::perspective(glm::radians(45.0f), selectedSwapExtent.width / (float)selectedSwapExtent.height, 0.1f, 10.0f);

  // Flip due to legacy opengl matrix inversion
  // should look into seeing whether this operation is faster on gpu side
  // or should it remain here as a cpu job
  /*
    Flip projection matrix as such
    proj[1][1] *= -1;
  */

  // Transfer data
  memcpy(m_UniformModelPtrs[imageIndex], &um, sizeof(UniformModelBuffer));
  return;
}

void GraphicsHandler::updateUniformVPBuffer(uint32_t imageIndex)
{
  UniformVPBuffer uvp;
  // Point left hand down to visualize the axis
  // Origin is at top left
  uvp.view = glm::lookAt(camera->getPosition(), camera->getPosition() + camera->DEFAULT_FORWARD_VECTOR, glm::vec3(0.0f, -1.0f, 0.0f));
  uvp.proj = glm::perspective(glm::radians(45.0f), selectedSwapExtent.width / (float)selectedSwapExtent.height, 0.1f, 100.0f);
  uvp.proj[1][1] *= -1;

  // Transfer
  memcpy(m_UniformVPPtrs[imageIndex], &uvp, sizeof(UniformVPBuffer));
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
  VkResult result;

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

  result = vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to create view into loaded texture");
  }

  return imageView;
}

/*
  THIS FUNCTION MUST BE CALLED AND PASSED MODELCLASS OBJECTS IN THE SAME ORDER THEY
  ARE CALLED IN ModelClass->loadModelData()

  loadModelData() GENERATES OFFSETS FROM WHAT THEY ARE GIVEN

  Loads vertex data into the buffer
*/
void GraphicsHandler::processModelData(ModelClass *modelObj)
{
  std::cout << "[+] Moving allocated model :: " << modelObj->typeName
            << " :: into gpu memory" << std::endl;

  VkResult result;

  VkBuffer vertexStagingBuffer{};
  VkDeviceMemory vertexStagingMemory{};

  VkBuffer indexStagingBuffer{};
  VkDeviceMemory indexStagingMemory{};

  // Create the vertex staging buffer
  createBuffer(modelObj->vertexDataSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               vertexStagingBuffer,
               vertexStagingMemory);

  // Map vertex staging
  void *vertexDataPtr;
  result = vkMapMemory(m_Device,
                       vertexStagingMemory,
                       0,
                       modelObj->vertexDataSize,
                       0,
                       &vertexDataPtr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to map memory for processing of model data");
  }
  // fill vertex stage
  memcpy(vertexDataPtr, modelObj->vertices.data(), modelObj->vertexDataSize);

  // Unmap vertex staging
  vkUnmapMemory(m_Device, vertexStagingMemory);

  // Copy vertex stage -> vertex buffer
  copyBuffer(vertexStagingBuffer, m_VertexBuffer, modelObj->vertexStartOffset, modelObj->vertexDataSize);
  std::cout << "Copying vertex data into buffer at dst offset -> " << modelObj->vertexStartOffset << std::endl;

  // create index stage
  createBuffer(modelObj->indexDataSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               indexStagingBuffer,
               indexStagingMemory);

  // Map index stage
  void *indexDataPtr;
  result = vkMapMemory(m_Device,
                       indexStagingMemory,
                       0,
                       modelObj->indexDataSize,
                       0,
                       &indexDataPtr);
  if (result != VK_SUCCESS)
  {
    G_EXCEPT("Failed to map memory for processing of model data");
  }

  // fill index stage
  memcpy(indexDataPtr, modelObj->indices.data(), modelObj->indexDataSize);

  // unmap index stage
  vkUnmapMemory(m_Device, indexStagingMemory);

  // Copy index stage -> index buffer
  copyBuffer(indexStagingBuffer, m_IndexBuffer, modelObj->indexStartOffset, modelObj->indexDataSize);
  std::cout << "Copying index data into buffer at dst offset -> " << modelObj->indexStartOffset << std::endl;

  // cleanup vertex stage
  vkDestroyBuffer(m_Device, vertexStagingBuffer, nullptr);
  vkFreeMemory(m_Device, vertexStagingMemory, nullptr);

  // cleanup index stage
  vkDestroyBuffer(m_Device, indexStagingBuffer, nullptr);
  vkFreeMemory(m_Device, indexStagingMemory, nullptr);
  return;
}

/*
  The index buffer and vertex buffer have been created

  This will load...
  vertex data into the vertex buffer AND
  index data into the index buffer AND
  MVP matrice data into an MVP buffer

  Vertex and Index data are device local
  MVP matrice data buffer is HOST_VISIBLE and can simply be written to each frame
*/
void GraphicsHandler::loadEntities(void)
{
  std::pair<int, int> prevBufferOffsets = {0, 0};
  std::pair<int, int> nextBufferOffsets = {0, 0};

  std::cout << "\tModel -> " << Human.typeName << std::endl;
  nextBufferOffsets = Human.loadModelData(prevBufferOffsets);
  std::cout << "Model Previous -> " << prevBufferOffsets.first << ", " << prevBufferOffsets.second << std::endl;
  std::cout << "Model next -> " << nextBufferOffsets.first << ", " << nextBufferOffsets.second << std::endl;

  if (nextBufferOffsets.first == -1 ||
      nextBufferOffsets.second == -1)
  {
    G_EXCEPT("Model data would exceed allocated buffer limits!");
  }

  // Update offset
  prevBufferOffsets = nextBufferOffsets;

  // Load grid vertices
  nextBufferOffsets = loadGridVertices(prevBufferOffsets);
  std::cout << "Grid Previous -> " << prevBufferOffsets.first << ", " << prevBufferOffsets.second << std::endl;
  std::cout << "Grid next -> " << nextBufferOffsets.first << ", " << nextBufferOffsets.second << std::endl;

  if (nextBufferOffsets.first == -1 ||
      nextBufferOffsets.second == -1)
  {
    G_EXCEPT("Grid vertices would exceed buffer limits!");
  }

  // update offset
  prevBufferOffsets = nextBufferOffsets;

  std::cout << "After grid -> " << prevBufferOffsets.first << ", " << prevBufferOffsets.second << std::endl;
  std::cout << "After grid -> " << nextBufferOffsets.first << ", " << nextBufferOffsets.second << std::endl;

  // After all model data is loaded we will pass those to an actual buffer
  processModelData(&Human);
  processGridData();
  return;
}

// Each entity requires 64 bytes of data for a model matrix
// Call before loading models
void GraphicsHandler::createUniformBuffers(void)
{
  VkResult result;

  // Resize UNIFORM buffers/memory containers

  // MODEL
  m_UniformModelBuffers.resize(m_SwapImages.size());
  m_UniformModelMemories.resize(m_SwapImages.size());
  m_UniformModelPtrs.resize(m_SwapImages.size());

  // V/P
  m_UniformVPBuffers.resize(m_SwapImages.size());
  m_UniformVPMemories.resize(m_SwapImages.size());
  m_UniformVPPtrs.resize(m_SwapImages.size());

  for (size_t i = 0; i < m_SwapImages.size(); i++)
  {
    // Model
    createBuffer(MAX_UNIFORM_BUFFER_SIZE,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 m_UniformModelBuffers[i],
                 m_UniformModelMemories[i]);

    // V/P
    createBuffer(MAX_UNIFORM_BUFFER_SIZE,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 m_UniformVPBuffers[i],
                 m_UniformVPMemories[i]);

    // Will be unmapped in destructor

    // Model ptrs
    result = vkMapMemory(m_Device, m_UniformModelMemories[i], 0, MAX_UNIFORM_BUFFER_SIZE, 0, &m_UniformModelPtrs[i]);
    if (result != VK_SUCCESS)
    {
      G_EXCEPT("Failed to map model buffer");
    }

    // V/p ptrs
    result = vkMapMemory(m_Device, m_UniformVPMemories[i], 0, MAX_UNIFORM_BUFFER_SIZE, 0, &m_UniformVPPtrs[i]);
    if (result != VK_SUCCESS)
    {
      G_EXCEPT("Failed to map view/project buffer");
    }
  }
  return;
}

void GraphicsHandler::createGridVertices(void)
{
  // X units from origin in length
  // From origin to X && From origin to -X
  int gridLength = 30;

  // Grid is flat against the X & Z PLANE
  for (int i = -gridLength; i < gridLength; i++)
  {
    // Rows
    grid.push_back(Vertex({{-gridLength, 0.0f, i}, {1.0f, 1.0f, 1.0f, 1.0f}}));
    grid.push_back(Vertex({{gridLength, 0.0f, i}, {1.0f, 1.0f, 1.0f, 1.0f}}));

    // Columns
    grid.push_back(Vertex({{i, 0.0f, -gridLength}, {0.0f, 0.0f, 1.0f, 1.0f}})); // negative z - blue
    grid.push_back(Vertex({{i, 0.0f, gridLength}, {0.0f, 1.0f, 0.0f, 1.0f}}));  // positive z - green
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

/* Must be called after create logical device! */
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

  // Bindings/layout
  if (m_DescriptorLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorLayout, nullptr);
  }

  /* Vertex buffer/memory */
  if (m_VertexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
  }
  if (m_VertexMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_Device, m_VertexMemory, nullptr);
  }

  /* Index buffer/memory */
  if (m_IndexBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
  }
  if (m_VertexMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_Device, m_IndexMemory, nullptr);
  }

  /* Uniform buffers */

  // UNMAP all MODEL MEMORIES
  if (!m_UniformModelMemories.empty())
  {
    for (auto &memory : m_UniformModelMemories)
    {
      vkUnmapMemory(m_Device, memory);
    }
  }

  // UNMAP all V/P MEMORIES
  if (!m_UniformVPMemories.empty())
  {
    for (auto &memory : m_UniformVPMemories)
    {
      vkUnmapMemory(m_Device, memory);
    }
  }

  // Destroy all MODEL BUFFERS
  if (!m_UniformModelBuffers.empty())
  {
    for (auto &buffer : m_UniformModelBuffers)
    {
      if (buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(m_Device, buffer, nullptr);
      }
    }
  }

  // Free all MODEL MEMORIES
  if (!m_UniformModelMemories.empty())
  {
    for (auto &memory : m_UniformModelMemories)
    {
      if (memory != VK_NULL_HANDLE)
      {
        vkFreeMemory(m_Device, memory, nullptr);
      }
    }
  }

  // Destroy all V/P BUFFERS
  if (!m_UniformVPBuffers.empty())
  {
    for (auto &buffer : m_UniformVPBuffers)
    {
      if (buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(m_Device, buffer, nullptr);
      }
    }
  }

  // Free all V/P MEMORIES
  if (!m_UniformVPMemories.empty())
  {
    for (auto &memory : m_UniformVPMemories)
    {
      if (memory != VK_NULL_HANDLE)
      {
        vkFreeMemory(m_Device, memory, nullptr);
      }
    }
  }

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
  if (m_Swap != VK_NULL_HANDLE && m_Swap != nullptr)
  {
    vkDestroySwapchainKHR(m_Device, m_Swap, nullptr);
  }
  return;
}