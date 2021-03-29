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

	std::unique_ptr<GraphicsHandler> gfx;
private:
	Display* display;
	Window window;
	XEvent event;
	int screen;
	bool running = true;
};

#define W_EXCEPT(string) throw Exception(__LINE__, __FILE__, string)