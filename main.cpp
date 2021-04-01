// Copyright 2021 .. fake
#include "WindowHandler.h"


int main(__attribute__((unused))int argc, __attribute__((unused))char *argv[]) {
  try {
    WindowHandler wnd(WINDOW_WIDTH, WINDOW_HEIGHT, "Bloody Day");
    wnd.go();
  }
  catch (ExceptionHandler& e) {
    std::cout << e.getType() << std::endl
              << e.getErrorDescription() << std::endl;
    std::exit(-1);
  }
  catch (std::exception& e) {
    std::cout << "Standard Library Exception"
              << std::endl << e.what() << std::endl;
    std::exit(-1);
  }
  catch (...) {
    std::cout << "Unhandled Exception" << std::endl
              << "Unhandled Exception! No further information available"
              << std::endl;
    std::exit(-1);
  }
  return 0;
}
