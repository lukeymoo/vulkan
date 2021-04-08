#pragma once

#include "ExceptionHandler.h"
#include "GraphicsHandler.h"

#include <memory>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

class WindowHandler
{
	public:
		class Exception : public ExceptionHandler
		{
			public:
				Exception(int line, std::string file, std::string message);
				~Exception(void);

				std::string getType(void);
		};
		WindowHandler(int w, int h, const char* title);
		~WindowHandler();

		void go(void);
		void handleXEvent(void);
		void draw(size_t currentFrame); // Calls GraphicsHandler to update buffers

		// Create a destroy event
		XEvent createEvent(const char* eventType);
		bool goodInit = true;
		std::unique_ptr<GraphicsHandler> gfx;
		Keyboard kbd;
		Mouse mouse;
	private:
		Display* display;
		Window window;
		XEvent event;
		int screen;
		bool running = true;
};

#define W_EXCEPT(string) throw Exception(__LINE__, __FILE__, string)
