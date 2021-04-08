# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Default target executed when no arguments are given to make.
default_target: all

.PHONY : default_target

# Allow only one "make -f Makefile2" at a time, but pass parallelism.
.NOTPARALLEL:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/luke/Documents/VulkanEngineCP

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/luke/Documents/VulkanEngineCP

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache

.PHONY : rebuild_cache/fast

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "No interactive CMake dialog available..."
	/usr/bin/cmake -E echo No\ interactive\ CMake\ dialog\ available.
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache

.PHONY : edit_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/luke/Documents/VulkanEngineCP/CMakeFiles /home/luke/Documents/VulkanEngineCP/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/luke/Documents/VulkanEngineCP/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean

.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named main

# Build rule for target.
main: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 main
.PHONY : main

# fast build rule for target.
main/fast:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/build
.PHONY : main/fast

Camera.o: Camera.cpp.o

.PHONY : Camera.o

# target to build an object file
Camera.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Camera.cpp.o
.PHONY : Camera.cpp.o

Camera.i: Camera.cpp.i

.PHONY : Camera.i

# target to preprocess a source file
Camera.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Camera.cpp.i
.PHONY : Camera.cpp.i

Camera.s: Camera.cpp.s

.PHONY : Camera.s

# target to generate assembly for a file
Camera.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Camera.cpp.s
.PHONY : Camera.cpp.s

ExceptionHandler.o: ExceptionHandler.cpp.o

.PHONY : ExceptionHandler.o

# target to build an object file
ExceptionHandler.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/ExceptionHandler.cpp.o
.PHONY : ExceptionHandler.cpp.o

ExceptionHandler.i: ExceptionHandler.cpp.i

.PHONY : ExceptionHandler.i

# target to preprocess a source file
ExceptionHandler.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/ExceptionHandler.cpp.i
.PHONY : ExceptionHandler.cpp.i

ExceptionHandler.s: ExceptionHandler.cpp.s

.PHONY : ExceptionHandler.s

# target to generate assembly for a file
ExceptionHandler.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/ExceptionHandler.cpp.s
.PHONY : ExceptionHandler.cpp.s

GraphicsHandler.o: GraphicsHandler.cpp.o

.PHONY : GraphicsHandler.o

# target to build an object file
GraphicsHandler.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/GraphicsHandler.cpp.o
.PHONY : GraphicsHandler.cpp.o

GraphicsHandler.i: GraphicsHandler.cpp.i

.PHONY : GraphicsHandler.i

# target to preprocess a source file
GraphicsHandler.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/GraphicsHandler.cpp.i
.PHONY : GraphicsHandler.cpp.i

GraphicsHandler.s: GraphicsHandler.cpp.s

.PHONY : GraphicsHandler.s

# target to generate assembly for a file
GraphicsHandler.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/GraphicsHandler.cpp.s
.PHONY : GraphicsHandler.cpp.s

Keyboard.o: Keyboard.cpp.o

.PHONY : Keyboard.o

# target to build an object file
Keyboard.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Keyboard.cpp.o
.PHONY : Keyboard.cpp.o

Keyboard.i: Keyboard.cpp.i

.PHONY : Keyboard.i

# target to preprocess a source file
Keyboard.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Keyboard.cpp.i
.PHONY : Keyboard.cpp.i

Keyboard.s: Keyboard.cpp.s

.PHONY : Keyboard.s

# target to generate assembly for a file
Keyboard.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Keyboard.cpp.s
.PHONY : Keyboard.cpp.s

Models.o: Models.cpp.o

.PHONY : Models.o

# target to build an object file
Models.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Models.cpp.o
.PHONY : Models.cpp.o

Models.i: Models.cpp.i

.PHONY : Models.i

# target to preprocess a source file
Models.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Models.cpp.i
.PHONY : Models.cpp.i

Models.s: Models.cpp.s

.PHONY : Models.s

# target to generate assembly for a file
Models.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Models.cpp.s
.PHONY : Models.cpp.s

Mouse.o: Mouse.cpp.o

.PHONY : Mouse.o

# target to build an object file
Mouse.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Mouse.cpp.o
.PHONY : Mouse.cpp.o

Mouse.i: Mouse.cpp.i

.PHONY : Mouse.i

# target to preprocess a source file
Mouse.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Mouse.cpp.i
.PHONY : Mouse.cpp.i

Mouse.s: Mouse.cpp.s

.PHONY : Mouse.s

# target to generate assembly for a file
Mouse.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Mouse.cpp.s
.PHONY : Mouse.cpp.s

Primitives.o: Primitives.cpp.o

.PHONY : Primitives.o

# target to build an object file
Primitives.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Primitives.cpp.o
.PHONY : Primitives.cpp.o

Primitives.i: Primitives.cpp.i

.PHONY : Primitives.i

# target to preprocess a source file
Primitives.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Primitives.cpp.i
.PHONY : Primitives.cpp.i

Primitives.s: Primitives.cpp.s

.PHONY : Primitives.s

# target to generate assembly for a file
Primitives.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/Primitives.cpp.s
.PHONY : Primitives.cpp.s

WindowHandler.o: WindowHandler.cpp.o

.PHONY : WindowHandler.o

# target to build an object file
WindowHandler.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/WindowHandler.cpp.o
.PHONY : WindowHandler.cpp.o

WindowHandler.i: WindowHandler.cpp.i

.PHONY : WindowHandler.i

# target to preprocess a source file
WindowHandler.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/WindowHandler.cpp.i
.PHONY : WindowHandler.cpp.i

WindowHandler.s: WindowHandler.cpp.s

.PHONY : WindowHandler.s

# target to generate assembly for a file
WindowHandler.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/WindowHandler.cpp.s
.PHONY : WindowHandler.cpp.s

main.o: main.cpp.o

.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i

.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s

.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) -f CMakeFiles/main.dir/build.make CMakeFiles/main.dir/main.cpp.s
.PHONY : main.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... rebuild_cache"
	@echo "... edit_cache"
	@echo "... main"
	@echo "... Camera.o"
	@echo "... Camera.i"
	@echo "... Camera.s"
	@echo "... ExceptionHandler.o"
	@echo "... ExceptionHandler.i"
	@echo "... ExceptionHandler.s"
	@echo "... GraphicsHandler.o"
	@echo "... GraphicsHandler.i"
	@echo "... GraphicsHandler.s"
	@echo "... Keyboard.o"
	@echo "... Keyboard.i"
	@echo "... Keyboard.s"
	@echo "... Models.o"
	@echo "... Models.i"
	@echo "... Models.s"
	@echo "... Mouse.o"
	@echo "... Mouse.i"
	@echo "... Mouse.s"
	@echo "... Primitives.o"
	@echo "... Primitives.i"
	@echo "... Primitives.s"
	@echo "... WindowHandler.o"
	@echo "... WindowHandler.i"
	@echo "... WindowHandler.s"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -S$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

