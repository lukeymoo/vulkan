 cmake_minimum_required(VERSION 3.10)

 project(VulkanEngine VERSION 1.0
            DESCRIPTION "HelloWorld cross platform Vulkan Engine"
            LANGUAGES CXX)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./build)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HEADERS 
    Headers/Primitives.h
    Headers/GraphicsHandler.h
    Headers/ExceptionHandler.h
    Headers/WindowHandler.h)

set(SOURCES 
    Primitives.cpp
    GraphicsHandler.cpp
    ExceptionHandler.cpp
    WindowHandler.cpp
    main.cpp)

add_compile_options(-Wall -std=c++20 -m64 -O2)

add_executable(main ${HEADERS} ${SOURCES})

target_include_directories(main PUBLIC Headers/)

target_link_libraries(main gcc vulkan dl pthread X11 Xxf86vm Xrandr Xi stdc++fs)
