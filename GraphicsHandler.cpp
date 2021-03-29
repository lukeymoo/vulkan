#include "GraphicsHandler.h"

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

GraphicsHandler::GraphicsHandler(Display* dsp, Window* wnd, int w, int h)
{
    display = dsp;
    window = wnd;
    windowWidth = w;
	windowHeight = h;

	m_PhysicalDevice = nullptr;
    m_Instance = nullptr;
    m_Device = nullptr;
    m_GraphicsQueue = nullptr;
    m_PresentQueue = nullptr;
    m_Surface = nullptr;
    m_Swap = nullptr;
    m_CommandPool = nullptr;
	m_VertexBuffer = nullptr;
	m_VertexMemory = nullptr;
    for(auto& semaphore : m_imageAvailableSemaphore) {
        semaphore = nullptr;
    }
    for(auto& semaphore : m_renderFinishedSemaphore) {
        semaphore = nullptr;
    }
    for(auto& fence : m_inFlightFences) {
        fence = nullptr;
    }
    for(auto& fence : m_imagesInFlight) {
        fence = nullptr;
    }
    for(auto& image : m_SwapImages) {
        image = nullptr;
    }
    for(auto& view : m_SwapViews) {
        view = nullptr;
    }
    for(auto& framebuffer : m_Framebuffers) {
        framebuffer = nullptr;
    }
    for(auto& commandBuffer : m_CommandBuffers) {
        commandBuffer = nullptr;
    }
    m_Pipeline = nullptr;
    m_PipelineLayout = nullptr;
    m_RenderPass = nullptr;
	m_SurfaceDetails = {};
	m_Debug = nullptr;

    vkCreateDebugUtilsMessengerEXT = nullptr;
	vkDestroyDebugUtilsMessengerEXT = nullptr;
	vkSubmitDebugUtilsMessageEXT = nullptr;

    for(auto& deviceInfo : deviceInfoList) {
        deviceInfo = {};
    }
    for(auto& queueInfo : queueCreateInfos) {
        queueInfo = {};
    }
    return;
}

GraphicsHandler::~GraphicsHandler()
{
    std::cout << "[+] Cleaning up Vulkan resources" << std::endl;
    // Objects must be destroyed in FILO order
	// Instance created first, upon which other handles are obtained so it is destroyed LAST
	
	vkDeviceWaitIdle(m_Device);
    vkQueueWaitIdle(m_GraphicsQueue);
    vkQueueWaitIdle(m_PresentQueue);

	cleanupSwapChain();

	// Cleanup buffer
	if(m_VertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	}
	// Now release buffer memory
	if(m_VertexMemory != VK_NULL_HANDLE) {
		vkFreeMemory(m_Device, m_VertexMemory, nullptr);
	}
	
	// Destroy sync objects
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_Device, m_imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(m_Device, m_renderFinishedSemaphore[i], nullptr);
		vkDestroyFence(m_Device, m_inFlightFences[i], nullptr);
	}

	// Destroy command pool
	if (m_CommandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	}
	if (m_Surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	}
	if (m_Device != VK_NULL_HANDLE) {
		vkDestroyDevice(m_Device, nullptr);
	}
	if (m_Debug != VK_NULL_HANDLE) {
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_Debug, nullptr);
	}
	if (m_Instance != VK_NULL_HANDLE) {
		vkDestroyInstance(m_Instance, nullptr);
	}

    std::cout << "[+] Vulkan cleaned up!" << std::endl;
	return;
}

void GraphicsHandler::initGraphics(void)
{
	/*
		Ensure all requested validation layers are supported
	*/
	if(enableValidationLayers) {
		std::cout << "[+] Debugging enabled" << std::endl;

		/*
			Returns false if fail, failList is populated with failed requests
		*/
		std::cout << "[+] Checking validation layers" << std::endl;
		std::string failList;
		bool validationCheck = checkValidationLayerSupport(&failList);
		if(!validationCheck) {
			std::string e = "The following requested validation layers were not found\n";
			e += failList;
			G_EXCEPT(e.c_str());
		}
	}

	/*
		Ensure system supports requested instance extensions here
	*/
	std::cout << "[+] Checking extensions" << std::endl;
	std::string failList;
	bool extensionCheck = checkInstanceExtensionSupport(&failList);

	// check if extension check failed
	if (!extensionCheck) {

		// if it failed, fetch the list of failed requests and throw
		std::string e = "The following requested instance extensions were not supported!\n";
		e += failList;
		G_EXCEPT(e.c_str());
	}

	/*
		creates instance, fetches physical device handle,
		describes graphics queue creation and creates logical device
	*/
	initVulkan();

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
void GraphicsHandler::initVulkan(void)
{
    /*
		Creates Vulkan instance and if debugging is enabled
		will load debug utility functions
	*/
    std::cout << "[+] Creating instance" << std::endl;
	createInstance();

	/*
		Selects an adapter we will create a logical device for
		in the process it also retrieves the index of graphics queue family
		and stores as member variable of deviceInfoList[selectedIndex]
	*/
    std::cout << "[+] Selecting adapter" << std::endl;
	if (!selectAdapter()) {
		G_EXCEPT("Failed to find a compatible device for rendering!");
	}

	// now that we've selected a physical device store its handle in m_PhysicalDevice
	// this is purely for convienience
	m_PhysicalDevice = deviceInfoList.at(selectedIndex).devHandle;

	/*
		Creates surface and DESCRIBES the creation of graphics queue and presentation queue
		If graphics queue and present queue share the same family index, only 1 is specified for creation

		Will also populate m_SurfaceDetails to describe the capabilities the surface supports in a swap chain
	*/
    std::cout << "[+] Creating Vulkan surface" << std::endl;
	registerSurface();


	// check for DEVICE extensions support
    std::cout << "[+] Checking device extension support" << std::endl;
	std::string failList;
	if (!checkDeviceExtensionSupport(&failList)) {
		std::string e = "The following requested device extensions were not supported!\n";
		e += failList;
		G_EXCEPT(e.c_str());
	}

    std::cout << "[+] Creating logical device and queues" << std::endl;
	createLogicalDeviceAndQueues();

	/*
		Creates swap chain, retreives images and then creates views for all of them
	*/
    std::cout << "[+] Creating swapchain" << std::endl;
	createSwapChain();

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
		Creates vertex buffer
		This will be vertex data is loaded for gpu rendering
	*/
	createVertexBuffer();

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

	// Load vertex buffer with vertices data
	void* data;
	vkMapMemory(m_Device, m_VertexMemory, 0, sizeof(vertices[0]) * vertices.size(), 0, &data);
	memcpy(data, vertices.data(), sizeof(vertices[0]) * vertices.size());
	vkUnmapMemory(m_Device, m_VertexMemory);

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createInstance(void)
{
	VkResult result;
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Bloody Day";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

	// configure validation layer debugging messages
	VkDebugUtilsMessengerCreateInfoEXT debuggerSettings{};
	debuggerSettings.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debuggerSettings.pNext = nullptr;
	debuggerSettings.flags = 0;
	debuggerSettings.messageSeverity =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debuggerSettings.messageType =	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debuggerSettings.pfnUserCallback = debugMessageProcessor;
	debuggerSettings.pUserData = nullptr;


	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = &debuggerSettings;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(requestedValidationLayers.size());
		createInfo.ppEnabledLayerNames = requestedValidationLayers.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requestedInstanceExtensions.size());
		createInfo.ppEnabledExtensionNames = requestedInstanceExtensions.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.enabledExtensionCount = 0;
		createInfo.ppEnabledExtensionNames = nullptr;
	}

	// Create Vulkan instance and store in m_Instance -- private member
	result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create Vulkan instance!"); }

	// we need to load debug utilities manually
	loadDebugUtils();

	result = vkCreateDebugUtilsMessengerEXT(m_Instance, &debuggerSettings, nullptr, &m_Debug);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create debug messenger"); }

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

// assigns `points` to all found devices with vulkan support
// sets `selectedIndex` to be the highest rated device
// `selectedIndex` is used to identify which device in vector deviceInfoList was selected
bool GraphicsHandler::selectAdapter(void)
{
	VkResult result;

	// Enumerate devices and find Vulkan support
	result = vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
	if (result != VK_SUCCESS) { G_EXCEPT("Failure enumerating devices!"); }

	if (deviceCount == 0) {
		G_EXCEPT("No devices found with Vulkan support!");
	}

	// Allocate space for device list
	deviceInfoList.resize(deviceCount);

	// Now we can fetch all the devices we found
	std::vector< VkPhysicalDevice> tempContainer(deviceCount);
	result = vkEnumeratePhysicalDevices(m_Instance, &deviceCount, tempContainer.data());
	if (result != VK_SUCCESS) { G_EXCEPT("Failure fetching physical devices!"); }

	// place the devices found in our custom structure
	for (size_t i = 0; i < deviceInfoList.size(); i++) {
		deviceInfoList.at(i).devHandle = tempContainer.at(i);
	}

	// iterate each device found
	for (size_t i = 0; i < deviceInfoList.size(); i++) {
		
		// prep structs
        memset(&deviceInfoList.at(i).devProperties, 0, sizeof(VkPhysicalDeviceProperties));
		memset(&deviceInfoList.at(i).devFeatures, 0, sizeof(VkPhysicalDeviceFeatures));

		// get infos
		/*
			must use `deviceInfoList` vector and `selectedIndex` as m_PhysicalDevice has not been
			assigned the handle of selected device yet

			m_PhysicalDevice can be used after rateDevice() has been successfully called
		*/
		vkGetPhysicalDeviceProperties(deviceInfoList.at(selectedIndex).devHandle, &deviceInfoList.at(i).devProperties);
		vkGetPhysicalDeviceFeatures(deviceInfoList.at(selectedIndex).devHandle, &deviceInfoList.at(i).devFeatures);

		// Check if dedicated gpu
		if (deviceInfoList.at(i).devProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			deviceInfoList.at(i).rating += 1000;
		}

		// add max image dimension to rating
		deviceInfoList.at(i).rating += deviceInfoList.at(i).devProperties.limits.maxImageDimension2D;

		// get supported queue families
		vkGetPhysicalDeviceQueueFamilyProperties(deviceInfoList.at(selectedIndex).devHandle,
			&deviceInfoList.at(i).queueFamilyCount, nullptr);
		// ensure this device supports at least 1 queue
		if (deviceInfoList.at(i).queueFamilyCount == 0) {
			deviceInfoList.at(i).rating = 0;
			continue;
		}

		// resize queue storage
		deviceInfoList.at(i).queueFamiles.resize(deviceInfoList.at(i).queueFamilyCount);
		// fetch all the queues and place into our custom struct
		vkGetPhysicalDeviceQueueFamilyProperties(deviceInfoList.at(selectedIndex).devHandle,
			&deviceInfoList.at(i).queueFamilyCount, deviceInfoList.at(i).queueFamiles.data());


		// ensure device supports VK_QUEUE_GRAPHICS_BIT
		bool hasGraphics = false;

		for (size_t j = 0; j < deviceInfoList.at(i).queueFamiles.size(); j++) {
			if (deviceInfoList.at(i).queueFamiles.at(j).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				// device supports graphics
				hasGraphics = true;
				deviceInfoList.at(i).graphicsFamilyIndex = j;
			}
		}
		// ensure it supports Graphics queue
		if (!hasGraphics) {
			deviceInfoList.at(i).rating = 0;
			continue;
		}

		// Ensure it has shaders
		if (!deviceInfoList.at(i).devFeatures.geometryShader) {
			deviceInfoList.at(i).rating = 0;
			continue;
		}
	}


	// -- SELECTION -- 
	// iterate deviceAndScore, select highest one
	selectedIndex = 0; // start with first device and move on
	for (size_t i = 0; i < deviceInfoList.size(); i++) {
		// if current index is higher than selected index, select new index
		if (deviceInfoList.at(i).rating > deviceInfoList.at(selectedIndex).rating) {
			selectedIndex = i;
		}
	}

	// double check our rating before returning
	if (deviceInfoList.at(selectedIndex).rating == 0) {
		// failed to find worthy device
		return false;
	}
	return true;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

// Creates surface from window system(winapi on windows)
void GraphicsHandler::registerSurface(void)
{
	VkResult result;

	// describe surface creation parameters
	VkXlibSurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
    createInfo.dpy = display;
    createInfo.window = *window;

	// Create window surface
	result = vkCreateXlibSurfaceKHR(m_Instance, &createInfo, nullptr, &m_Surface);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create window surface!"); }

	// describe creation of graphics queue
	VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.pNext = nullptr;
	graphicsQueueCreateInfo.flags = 0;
	graphicsQueueCreateInfo.queueFamilyIndex = deviceInfoList.at(selectedIndex).graphicsFamilyIndex;
	graphicsQueueCreateInfo.queueCount = 1;
	graphicsQueueCreateInfo.pQueuePriorities = &queuePrio;

	// push this description to our vector
	queueCreateInfos.push_back(graphicsQueueCreateInfo);

	// check if the earlier fetched queue family supports presentation
	VkBool32 hasPresentSupport = false;
	result = vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice,
		deviceInfoList.at(selectedIndex).graphicsFamilyIndex, m_Surface, &hasPresentSupport);
	if (result != VK_SUCCESS) { G_EXCEPT("Failure checking for presentation support!"); }

	if (hasPresentSupport) {
		deviceInfoList.at(selectedIndex).presentFamilyIndex = deviceInfoList.at(selectedIndex).graphicsFamilyIndex;
	}
	else {
		// selected device's graphics queue does not have present support
		// maybe look for another queue
		G_EXCEPT("Selected device supports graphics queues but was not found to support presentation!");
	}

	// if graphics queue and present queue do NOT share an index, create present queue
	if (deviceInfoList.at(selectedIndex).graphicsFamilyIndex != deviceInfoList.at(selectedIndex).presentFamilyIndex) {

		// describe creation of presentation queue
		VkDeviceQueueCreateInfo presentQueueCreateInfo{};
		presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		presentQueueCreateInfo.pNext = nullptr;
		presentQueueCreateInfo.flags = 0;
		presentQueueCreateInfo.queueFamilyIndex = deviceInfoList.at(selectedIndex).presentFamilyIndex;
		presentQueueCreateInfo.queueCount = 1;
		presentQueueCreateInfo.pQueuePriorities = &queuePrio;

		// push description into vector
		queueCreateInfos.push_back(presentQueueCreateInfo);

	}

	/*
		Ensure selected device supports at least 1 present mode
		and 1 image format for surfaces
	*/
	m_SurfaceDetails = querySwapChainSupport();

	if (m_SurfaceDetails.formats.empty() || m_SurfaceDetails.presentModes.empty()) {
		G_EXCEPT("Selected device does not support any image formats or presentation modes for it's surfaces!");
	}

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createLogicalDeviceAndQueues(void)
{
	VkResult result;

	// logical device creation needs to occur after preparing creation of all queues
	// now that we've described all queues we want to create, past into creation of logical device
	VkDeviceCreateInfo logicalDeviceCreateInfo{};
	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); // -- pass here
	logicalDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	logicalDeviceCreateInfo.pEnabledFeatures = &deviceInfoList.at(selectedIndex).devFeatures;
	logicalDeviceCreateInfo.enabledLayerCount = 0; // device level layers deprecated
	logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr; // device level layers deprecated
	logicalDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requestedDeviceExtensions.size());
	logicalDeviceCreateInfo.ppEnabledExtensionNames = requestedDeviceExtensions.data();


	// Create logical device
	result = vkCreateDevice(m_PhysicalDevice, &logicalDeviceCreateInfo, nullptr, &m_Device);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create logical device!"); }

	// Retrieve the Graphics queue from newly created logical device
	vkGetDeviceQueue(m_Device, deviceInfoList.at(selectedIndex).graphicsFamilyIndex, 0, &m_GraphicsQueue);

	// Check if the presentation/graphics queue share an index before requesting both
	// if not request present queue
	if (deviceInfoList.at(selectedIndex).graphicsFamilyIndex != deviceInfoList.at(selectedIndex).presentFamilyIndex) {
		vkGetDeviceQueue(m_Device, deviceInfoList.at(selectedIndex).presentFamilyIndex, 0, &m_PresentQueue);
	}
	else {
		// since the indexes are the same we will point present queue to graphics
		m_PresentQueue = m_GraphicsQueue;
	}

	// Ensure we retrieved the queue
	if (!m_GraphicsQueue) {
		G_EXCEPT("Failed to retrieve handle to graphics command queue");
	}
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

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
	if (m_SurfaceDetails.capabilities.maxImageCount > 0 && imageCount > m_SurfaceDetails.capabilities.maxImageCount) {
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
	if (deviceInfoList.at(selectedIndex).presentFamilyIndex == deviceInfoList.at(selectedIndex).graphicsFamilyIndex) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	else {
		uint32_t queueIndices[] = {
			deviceInfoList.at(selectedIndex).presentFamilyIndex,
			deviceInfoList.at(selectedIndex).graphicsFamilyIndex
		};
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueIndices;
	}
	swapChainCreateInfo.preTransform = m_SurfaceDetails.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = swapPresentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = nullptr;

	// Create the swap chain
	result = vkCreateSwapchainKHR(m_Device, &swapChainCreateInfo, nullptr, &m_Swap);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create swap chain!"); }

	// retrieve swap chain images that were created
	// could be higher than our specified amount in `minImageCount`


	uint32_t swapImageCount = 0;

	// get count of images
	result = vkGetSwapchainImagesKHR(m_Device, m_Swap, &swapImageCount, nullptr);
	if (result != VK_SUCCESS) { G_EXCEPT("Error getting count of swap chain images"); }

	// ensure non zero
	if (swapImageCount == 0) { G_EXCEPT("Swapchain did not create any images!"); }

	// get handles of images for purposes of creating views for them
	m_SwapImages.resize(swapImageCount);
	result = vkGetSwapchainImagesKHR(m_Device, m_Swap, &swapImageCount, m_SwapImages.data());
	if (result != VK_SUCCESS) { G_EXCEPT("Error getting swap chain image handles!"); }

	// resize views container to be the same as swap images container
	m_SwapViews.resize(swapImageCount);

	for (size_t i = 0; i < m_SwapImages.size(); i++) {
		// describe view for image
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.pNext = nullptr;
		viewCreateInfo.flags = 0;
		viewCreateInfo.image = m_SwapImages[i];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = selectedSwapFormat.format;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		// Create the image view
		result = vkCreateImageView(m_Device, &viewCreateInfo, nullptr, &m_SwapViews[i]);
		if (result != VK_SUCCESS) { G_EXCEPT("Error creating swap view"); }
	}

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createGraphicsPipeline(void)
{
	VkResult result;

	auto vertexShader = readFile("shaders/vert.spv");
	auto fragShader = readFile("shaders/frag.spv");

	// ensure buffers not empty
	if (vertexShader.empty()) {
		G_EXCEPT("Failed to load vertex shader!");
	}
	if (vertexShader.empty()) {
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
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.pNext = nullptr;
	vertexShaderStageInfo.flags = 0;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = vertexModule;
	vertexShaderStageInfo.pName = "main";
	vertexShaderStageInfo.pSpecializationInfo = nullptr;

	// describe fragment shader stage
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.pNext = nullptr;
	fragmentShaderStageInfo.flags = 0;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragModule;
	fragmentShaderStageInfo.pName = "main";
	fragmentShaderStageInfo.pSpecializationInfo = nullptr;


	// store creation descriptions in array
	VkPipelineShaderStageCreateInfo pipelineStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };


	// describe creation of vertex input stage
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescription = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

	// describe the type of geometry vulkan will be drawing
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	// configure viewport ( what portion of frame buffer to render to )
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float)selectedSwapExtent.width;
	viewport.height = (float)selectedSwapExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// configure scissor ( what portion of framebuffer to send to output merger )
	// use entire frame buffer
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = selectedSwapExtent;

	// combine viewport and scissor into a vulkan viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
	rasterInfo.rasterizerDiscardEnable = VK_FALSE; // true disables output to frame buffer
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL; // wireframe vs fill
	rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterInfo.depthBiasEnable = VK_FALSE;
	rasterInfo.depthBiasConstantFactor = 0.0f;
	rasterInfo.depthBiasClamp = 0.0f;
	rasterInfo.depthBiasSlopeFactor = 0.0f;
	rasterInfo.lineWidth = 1.0f;

	// configure multisampling
	VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
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
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
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
	colorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	

	// global color blending constants
	VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
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
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	// Create pipeline layout
	result = vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create pipeline layout!"); }

	// defines render pass behavior
	createRenderPass();

	// describe pipeline object creation
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = pipelineStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterInfo;
	pipelineInfo.pMultisampleState = &multisamplingInfo;
	pipelineInfo.pDepthStencilState = &depthStencilInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlendingInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0; // sub pass index
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create graphics pipeline object!"); }


	vkDestroyShaderModule(m_Device, vertexModule, nullptr);
	vkDestroyShaderModule(m_Device, fragModule, nullptr);
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

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
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create render pass object!"); }

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createFrameBuffers(void)
{
	VkResult result;

	// resize framebuffers to have enough for each swapchain image views
	m_Framebuffers.resize(m_SwapViews.size());

	// iterate each swap chain image and create frame buffer for it
	for	(size_t i = 0; i < m_SwapViews.size(); i++) {
		VkImageView attachments[] = {
			m_SwapViews[i]
		};
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

		result = vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]);
		if (result != VK_SUCCESS) { G_EXCEPT("Failed to create frame buffer for image view"); }
	}

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createCommandPool(void)
{
	VkResult result;

	VkCommandPoolCreateInfo commandPoolInfo{};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = 0; // lookup flags for better control
	// configure command pool to store graphics commands
	commandPoolInfo.queueFamilyIndex = deviceInfoList.at(selectedIndex).graphicsFamilyIndex;

	result = vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_CommandPool);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create graphics command pool"); }
	
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createVertexBuffer(void)
{
	VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

	// Helper function that creates the buffer and allocates memory for it
	createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertexBuffer, m_VertexMemory);
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

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

	result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data());
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to allocate command buffers!"); }

	// Begin recording command buffer
	for (size_t i = 0; i < m_CommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		result = vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo);
		if (result != VK_SUCCESS) { G_EXCEPT("Failed to begin command buffer!"); }

		// Begin render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext = nullptr;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_Framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = selectedSwapExtent;

		VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		// Bind vertex buffers
		VkBuffer vertexBuffers[] = { m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_CommandBuffers[i]);

		result = vkEndCommandBuffer(m_CommandBuffers[i]);
		if (result != VK_SUCCESS) { G_EXCEPT("Error ending command buffer recording"); }

	}

	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

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
	// by default created in unsignaled state which causes a hang at start of draw portion of game loop
	// due to waiting for a fence that was never submitted so we use this flag to simulate that
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// Create semaphores for each swap chain image
	m_imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_imagesInFlight.resize(m_SwapImages.size(), VK_NULL_HANDLE);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphore[i]);
		if (result != VK_SUCCESS) { G_EXCEPT("Failed to create image semaphore!"); }

		result = vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphore[i]);
		if (result != VK_SUCCESS) { G_EXCEPT("Failed to create render semaphore"); }

		result = vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_inFlightFences[i]);
		if (result != VK_SUCCESS) { G_EXCEPT("Failed to create fences"); }
	}
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

// Searches for specified layers(Global variable validationLayers)
// and returns whether they were all available or not
bool GraphicsHandler::checkValidationLayerSupport(std::string* failList)
{
	uint32_t layerCount = 0;
	failList->clear();
	VkResult result;
	result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	if (result != VK_SUCCESS) {
		G_EXCEPT("There was an error getting instance layer count!");
	}
	
	// Ensure we found layers
	if (!layerCount) {
		*failList = "Layer check could not find any available layers on system!";
		return false;
	}

	std::vector<VkLayerProperties> availableLayers(layerCount);
	result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	if (result != VK_SUCCESS) {
		G_EXCEPT("There was an error getting instance layer list!");
	}

	// iterates through list of requested layers
	for (const char* layerName : requestedValidationLayers) {
		bool layerFound = false;

		// iterates through list of fetched supported layers
		for (const auto& fetchedLayer : availableLayers) {
			// check if we have a match between fetched layer and requested one
			if (strcmp(layerName, fetchedLayer.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		// if we made it through fetched list without finding layer
		// add to failure list
		if (!layerFound) {
			*failList += layerName;
			*failList += "\n";
		}

	}

	// if failList is not empty return errors
	// errors added to fail list as they occur so just return here
	if (!failList->empty()) {
		return false;
	}
	else {
		return true;
	}
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
bool GraphicsHandler::checkInstanceExtensionSupport(std::string* failList)
{
	uint32_t instanceExtensionCount = 0;
	VkResult result;
	failList->clear(); // ensure failList is clear

	// get the number of supported instance extensions
	result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
	if (result != VK_SUCCESS) {
		G_EXCEPT("There was an error getting instance extension count!");
	}

	// ensure we found any extensions
	if (!instanceExtensionCount) {
		*failList = "Extension check could not find any available extensions on system";
		return false;
	}

	// fetch all the supported instances
	std::vector<VkExtensionProperties> availableInstanceExtensions(instanceExtensionCount);
	result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, availableInstanceExtensions.data());
	if (result != VK_SUCCESS) {
		G_EXCEPT("There was an error getting instance extension list!");
	}

	// iterate requested INSTANCE extensions
	for (const auto& requestedExtensions : requestedInstanceExtensions) {
		bool extensionMatch = false;

		// iterate the list of all supported INSTANCE extensions
		for (const auto& fetchedExtensions : availableInstanceExtensions) {
			// check if match
			if (strcmp(requestedExtensions, fetchedExtensions.extensionName) == 0) {
				extensionMatch = true;
				break;
			}
		}

		// if failed to find match, add to failList
		if (!extensionMatch) {
			*failList += requestedExtensions;
			*failList += "\n";
		}

	}

	// if failList is not empty return failure
	if (!failList->empty()) {
		return false;
	}

	return true;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

// check for DEVICE level extensions
bool GraphicsHandler::checkDeviceExtensionSupport(std::string* failList)
{
	uint32_t deviceExtensionCount = 0;
	VkResult result;
	failList->clear(); // ensure failList is clear

	// get total number of supported DEVICE extensions
	result = vkEnumerateDeviceExtensionProperties(deviceInfoList.at(selectedIndex).devHandle, nullptr, &deviceExtensionCount, nullptr);
	if (result != VK_SUCCESS) {
		G_EXCEPT("There was an error getting count of device extensions!");
	}

	std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
	// fetch list of all supported DEVICE extensions
	result = vkEnumerateDeviceExtensionProperties(
		deviceInfoList.at(selectedIndex).devHandle,
		nullptr,
		&deviceExtensionCount,
		availableDeviceExtensions.data());
	if (result != VK_SUCCESS) {
		G_EXCEPT("There was an error getting device extension list!");
	}

	// iterate list of requested DEVICE extensions
	for (const auto& requestedExtension : requestedDeviceExtensions) {
		bool extensionMatch = false;
		// iterate list of supported DEVICE extensions
		for (const auto& fetchedDeviceExtension : availableDeviceExtensions) {
			if (strcmp(requestedExtension, fetchedDeviceExtension.extensionName) == 0) {
				extensionMatch = true;
				break;
			}
		}

		// if failed to find requested DEVICE extension
		// add to failList
		if (!extensionMatch) {
			*failList += requestedExtension;
			*failList += "\n";
		}


	}


	return true;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

bool GraphicsHandler::loadDebugUtils(void)
{
	PFN_vkVoidFunction temp_fp;

	// load vkCreateDebugUtilsMessengerEXT
	temp_fp = vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
	if (!temp_fp) { G_EXCEPT("Failure loading vkCreateDebugUtilsMessengerEXT"); }
	vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(temp_fp);

	// load vkDestroyDebugUtilsMessengerEXT
	temp_fp = vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
	if (!temp_fp) { G_EXCEPT("Failure loading vkDestroyDebugUtilsMessengerEXT"); }
	vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(temp_fp);

	// load vkSubmitDebugUtilsMessageEXT
	temp_fp = vkGetInstanceProcAddr(m_Instance, "vkSubmitDebugUtilsMessageEXT");
	if (!temp_fp) { G_EXCEPT("Failure loading vkSubmitDebugUtilsMessageEXT"); }
	vkSubmitDebugUtilsMessageEXT = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(temp_fp);

	return true;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

SwapChainSupportDetails GraphicsHandler::querySwapChainSupport(void)
{
	SwapChainSupportDetails details;
	VkResult result;

	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &details.capabilities);
	if (result != VK_SUCCESS) { G_EXCEPT("There was an error getting surface swap chain capabilities!"); }

	// get supported formats
	uint32_t formatCount = 0;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
	if (result != VK_SUCCESS) { G_EXCEPT("Error finding supported formats for surface!"); }

	// ensure formats exist
	if (formatCount == 0) { G_EXCEPT("No supported formats found for surface!"); }

	// get list of supported surface formats
	details.formats.resize(formatCount);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, details.formats.data());
	if (result != VK_SUCCESS) { G_EXCEPT("There was an error getting supported formats list!"); }

	uint32_t presentModeCount = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
	if (result != VK_SUCCESS) { G_EXCEPT("Error getting supported present mode count!"); }

	// ensure present modes exist
	if (presentModeCount == 0) { G_EXCEPT("Failed to find any supported present modes!"); }

	// get list of surface present modes
	details.presentModes.resize(presentModeCount);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, details.presentModes.data());
	if (result != VK_SUCCESS) { G_EXCEPT("Error getting supported surface presentation modes!"); }

	return details;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

VkShaderModule GraphicsHandler::createShaderModule(const std::vector<char>& code)
{
	VkResult result;
	VkShaderModule module;

	VkShaderModuleCreateInfo moduleInfo{};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.pNext = nullptr;
	moduleInfo.flags = 0;
	moduleInfo.codeSize = code.size();
	moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	result = vkCreateShaderModule(m_Device, &moduleInfo, nullptr, &module);
	if (result != VK_SUCCESS) { G_EXCEPT("Failed to create shader module!"); }

	return module;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageProcessor(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data)
{
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::ostringstream oss;
		oss << std::endl << "Warning: " << callback_data->messageIdNumber << ", " << callback_data->pMessageIdName
			<< std::endl << callback_data->pMessage << std::endl << std::endl;
		std::cout << oss.str();
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		/*std::ostringstream oss;
		oss << std::endl << "Verbose message : " << callback_data->messageIdNumber << ", " << callback_data->pMessageIdName
			<< std::endl << callback_data->pMessage << std::endl << std::endl;
		OutputDebugString(oss.str().c_str());*/
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		std::ostringstream oss;
		oss << std::endl << "Error: " << callback_data->messageIdNumber << ", " << callback_data->pMessageIdName
			<< std::endl << callback_data->pMessage << std::endl << std::endl;
		std::cout << oss.str();
	}
	return VK_FALSE;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

/*
	Iterates through supported formats found when querying the swap chain
	Selects VK_FORMAT_B8G8R8A8_SRGB if it exists as it is one of the most common formats for images
	and non linear sRGB as it is also the most common
*/
VkSurfaceFormatKHR GraphicsHandler::chooseSwapChainFormat(void)
{
	for (const auto& availableFormat : m_SurfaceDetails.formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return m_SurfaceDetails.formats[0];
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

VkPresentModeKHR GraphicsHandler::chooseSwapChainPresentMode(void)
{
	// application will prefer MAILBOX present mode as it is akin to triple buffering with less latency
	for (const auto& presentMode : m_SurfaceDetails.presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; // failsafe as it is guaranteed to be available
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

/*
	Set resolution of swap chain images
	should be equal to render area of window
*/
VkExtent2D GraphicsHandler::chooseSwapChainExtent(void)
{
	if (m_SurfaceDetails.capabilities.currentExtent.width != UINT32_MAX) {
		return m_SurfaceDetails.capabilities.currentExtent;
	}
	else {
        Window rw;
        int rx, ry; // returned x/y
        uint rwidth, rheight; // returned width/height
        uint rborder; // returned  border width
        uint rdepth; // returned bit depth
		if(!XGetGeometry(display, *window, &rw, &rx, &ry, &rwidth, &rheight, &rborder, &rdepth)) {
            G_EXCEPT("[-] Failed to get window geometry!");
        }

		VkExtent2D actualExtent;
		actualExtent.width = static_cast<uint32_t>(rwidth);
		actualExtent.height = static_cast<uint32_t>(rheight);

		// do some min max clamping
		actualExtent.width = std::max(
			m_SurfaceDetails.capabilities.minImageExtent.width,
			std::min(m_SurfaceDetails.capabilities.maxImageExtent.width, actualExtent.width)
		);

		actualExtent.height = std::max(
			m_SurfaceDetails.capabilities.minImageExtent.height,
			std::min(m_SurfaceDetails.capabilities.maxImageExtent.height, actualExtent.height)
		);
		return actualExtent;
	}
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

std::vector<char> GraphicsHandler::readFile(std::string filename)
{
	size_t fileSize;
	std::ifstream file;
	std::vector<char> buffer;

	// check if file exists
	if(!std::filesystem::exists(filename)) {
		std::ostringstream oss;
		oss << "Specified file does not exist -> " << filename << std::endl;
		G_EXCEPT(oss.str());
	}

	file.open(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
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

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::cleanupSwapChain(void)
{
	// Clean up frame buffers
	for (const auto& buffer : m_Framebuffers) {
		vkDestroyFramebuffer(m_Device, buffer, nullptr);
	}

    if(!m_Framebuffers.empty()) {
        vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_Framebuffers.size()), m_CommandBuffers.data());
    }

	// Destroy pipeline object
	if (m_Pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
	}
	// Destroy pipeline layout
	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	}
	// Destroy render pass object
	if (m_RenderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	}
	// ensure we destroy all views to swap chain images
	if (!m_SwapViews.empty()) {
		for (const auto& view : m_SwapViews) {
			vkDestroyImageView(m_Device, view, nullptr);
		}
	}
	if (m_Swap != VK_NULL_HANDLE && m_Swap != nullptr) {
		vkDestroySwapchainKHR(m_Device, m_Swap, nullptr);
	}
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::recreateSwapChain(void)
{
	vkDeviceWaitIdle(m_Device);

	/*
		Ensure resources weren't previously destroyed before
		attempting to do so again
	*/
	if (SHOULD_RENDER) {
		// free up binded resources for recreation
		cleanupSwapChain();
	}

	// Get new details
    std::cout << "[/] Creating new swap chain" << std::endl;
	m_SurfaceDetails = querySwapChainSupport();

	if (m_SurfaceDetails.formats.empty() || m_SurfaceDetails.presentModes.empty()) {
		G_EXCEPT("Selected device does not support any image formats or presentation modes for it's surfaces!");
	}
	
	// if any dimensions are 0, skip resource creation
	if (m_SurfaceDetails.capabilities.currentExtent.width == 0 ||
			m_SurfaceDetails.capabilities.currentExtent.height == 0) {
		SHOULD_RENDER = false;
		return;
	}
	else {
		SHOULD_RENDER = true;
	}

	// Creates swapchain and image views
	createSwapChain();

	createGraphicsPipeline();

	createFrameBuffers();

	createCommandBuffers();
	std::cout << "[+] Done!" << std::endl;
	return;
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

uint32_t GraphicsHandler::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &deviceMemoryProperties);

	for(uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
		if(typeFilter & (1 << i) &&
			(deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	G_EXCEPT("Failed to find suitable VRAM type");
}

/*
    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/

void GraphicsHandler::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
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

	// Create vertex buffer
	result = vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer);
	if(result != VK_SUCCESS) { G_EXCEPT("Failed to create vertex buffer!"); }

	// allocate memory for buffer
	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

	// Describe memory allocation
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	// Allocate the memory
	result = vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory);
	if(result != VK_SUCCESS) { G_EXCEPT("Failed to allocate memory for vertex buffer!"); }

	// Now bind the memory to the buffer
	result = vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
	if(result != VK_SUCCESS) { G_EXCEPT("Failed to bind memory to vertex buffer"); }
	return;
}