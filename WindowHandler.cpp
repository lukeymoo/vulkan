#include "WindowHandler.h"

WindowHandler::Exception::Exception(int l, std::string f, std::string description)
 : ExceptionHandler(l, f, description)
{
    type = "Window Handler Exception";
    errorDescription = description;
    return;
}

WindowHandler::Exception::~Exception(void)
{
    return;
}

/* Create window and initialize Vulkan */
WindowHandler::WindowHandler(int w, int h, const char* title)
{

    // Open connection xserver
    display = XOpenDisplay(NULL);
    if(display == NULL) {
        W_EXCEPT("Failed to open connection to xserver");
    }

    screen = DefaultScreen(display);

    // Create window
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, w, h, 1, WhitePixel(display, screen), BlackPixel(display, screen));
    
    Atom del_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &del_window, 1);

    // Configure to handle specified events
    XSelectInput(display, window, ExposureMask | KeyPressMask);

    XStoreName(display, window, title);

    // Display window
    XMapWindow(display, window);

    // Create the graphics
    gfx = std::make_unique<GraphicsHandler>(display, &window, WINDOW_WIDTH, WINDOW_HEIGHT);

	// For some reason exceptions don't make it up to main()
	// on linux
	try
	{
		// initialize graphics
		gfx->initGraphics();
	}
	catch (ExceptionHandler& e) {
		std::cout << e.getType() << std::endl << e.getErrorDescription() << std::endl;
		std::exit(-1);
	}
	catch (std::exception& e) {
		std::cout << "Standard Library Exception" << std::endl << e.what() << std::endl;
		std::exit(-1);
	}
	catch (...) {
		std::cout << "Unhandled Exception" << std::endl
			<< "Unhandled Exception! No further information available" << std::endl;
		std::exit(-1);
	}
	

    return;
}

WindowHandler::~WindowHandler(void)
{
	std::cout << "[+] Calling release of graphics resources" << std::endl;
	/*
		!! Vulkan cannot be properly cleaned up if X11 resources are released beforehand !!
		Call Vulkan resource destruction NOW
	*/
	gfx.reset();
	std::cout << "[+] Cleaning up window resources" << std::endl;
	XDestroyWindow(display, window);
	XCloseDisplay(display);
	return;
}

void WindowHandler::go(void)
{
	size_t currentFrame = 0;

	while(running) {
	/*
	Draw commands here
	*/
	VkResult result;
	uint32_t imageIndex;

	// wait for fence
	vkWaitForFences(gfx->m_Device, 1, &gfx->m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	/* -- DRAW BEGINS HERE -- */


	currentFrame = (currentFrame + 1) & MAX_FRAMES_IN_FLIGHT;

	result = vkAcquireNextImageKHR(gfx->m_Device, gfx->m_Swap,
	UINT64_MAX, gfx->m_imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// Do some case checking
	switch (result) {
		case VK_ERROR_OUT_OF_DATE_KHR:
			std::cout << "[+] Next image needs new swap" << std::endl;
			gfx->recreateSwapChain();
			// Will skip handling x11 events this iteration
			// reset loop
			continue;
			break;
		case VK_SUBOPTIMAL_KHR:
			std::cout << "[/] Swapchain is no longer optimal" << std::endl;
			break;
		case VK_SUCCESS:
		break;
		default: // unhandled error occurred
			W_EXCEPT("There was an unhandled exception while acquiring a swapchain image for rendering!");
			break;
	}

	// Update uniform buffer before draw
	gfx->updateUniformBuffer(imageIndex);

	// If image still use, wait for it
	if (gfx->m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(gfx->m_Device, 1, &gfx->m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Now mark the image as in use
	gfx->m_imagesInFlight[imageIndex] = gfx->m_inFlightFences[currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { gfx->m_imageAvailableSemaphore[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &gfx->m_CommandBuffers[imageIndex];
	VkSemaphore renderSemaphores[] = { gfx->m_renderFinishedSemaphore[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = renderSemaphores;

	vkResetFences(gfx->m_Device, 1, &gfx->m_inFlightFences[currentFrame]);

	result = vkQueueSubmit(gfx->m_GraphicsQueue, 1, &submitInfo, gfx->m_inFlightFences[currentFrame]);
	if (result != VK_SUCCESS) { W_EXCEPT("Failed to submit draw command buffer!"); }

	// Presentation
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderSemaphores;
	VkSwapchainKHR swapchains[] = { gfx->m_Swap };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(gfx->m_PresentQueue, &presentInfo);
	// Do some case checking
	switch (result) {
		case VK_ERROR_OUT_OF_DATE_KHR:
			std::cout << "[+] Presentation needs new swap" << std::endl;
			gfx->recreateSwapChain();
			// reset loop -- will skip handling x11 events this iteration
			continue;
			break;
		case VK_SUBOPTIMAL_KHR:
			std::cout << "[/] Swapchain is no longer optimal" << std::endl;
			break;
		case VK_SUCCESS:
		break;
		default: // unhandled error occurred
			W_EXCEPT("There was an unhandled exception while acquiring a swapchain image for rendering!");
			break;
	}

	XNextEvent(display, &event);

	switch(event.type) {
		case KeyPress:
			if(XLookupKeysym(&event.xkey, 0) == XK_Escape) {
				// Generate a close event for graceful exit
				XEvent cev;
				cev.xclient.type = ClientMessage;
				cev.xclient.window = window;
				cev.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", true);
				cev.xclient.format = 32;
				cev.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", false);
				cev.xclient.data.l[1] = CurrentTime;
				XSendEvent(display, window, false, ExposureMask, &cev);
			}
			break;
		case Expose:
		break;
		// configured to only capture WM_DELETE_WINDOW so we exit here
		case ClientMessage:
			std::cout << "[+] Exit requested" << std::endl;
			running = false;
			break;
		}
	} // end loop

	return;
}
