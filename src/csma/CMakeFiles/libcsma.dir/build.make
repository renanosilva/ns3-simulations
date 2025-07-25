# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/renan/ns-allinone-3.42/ns-3.42

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/renan/ns-allinone-3.42/ns-3.42

# Include any dependencies generated for this target.
include src/csma/CMakeFiles/libcsma.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/csma/CMakeFiles/libcsma.dir/compiler_depend.make

# Include the progress variables for this target.
include src/csma/CMakeFiles/libcsma.dir/progress.make

# Include the compile flags for this target's objects.
include src/csma/CMakeFiles/libcsma.dir/flags.make

# Object files for target libcsma
libcsma_OBJECTS =

# External object files for target libcsma
libcsma_EXTERNAL_OBJECTS = \
"/home/renan/ns-allinone-3.42/ns-3.42/src/csma/CMakeFiles/libcsma-obj.dir/helper/csma-helper.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/csma/CMakeFiles/libcsma-obj.dir/model/backoff.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/csma/CMakeFiles/libcsma-obj.dir/model/csma-channel.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/csma/CMakeFiles/libcsma-obj.dir/model/csma-net-device.cc.o"

build/lib/libns3.42-csma-default.so: src/csma/CMakeFiles/libcsma-obj.dir/helper/csma-helper.cc.o
build/lib/libns3.42-csma-default.so: src/csma/CMakeFiles/libcsma-obj.dir/model/backoff.cc.o
build/lib/libns3.42-csma-default.so: src/csma/CMakeFiles/libcsma-obj.dir/model/csma-channel.cc.o
build/lib/libns3.42-csma-default.so: src/csma/CMakeFiles/libcsma-obj.dir/model/csma-net-device.cc.o
build/lib/libns3.42-csma-default.so: src/csma/CMakeFiles/libcsma.dir/build.make
build/lib/libns3.42-csma-default.so: src/csma/CMakeFiles/libcsma.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/renan/ns-allinone-3.42/ns-3.42/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking CXX shared library ../../build/lib/libns3.42-csma-default.so"
	cd /home/renan/ns-allinone-3.42/ns-3.42/src/csma && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/libcsma.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/csma/CMakeFiles/libcsma.dir/build: build/lib/libns3.42-csma-default.so
.PHONY : src/csma/CMakeFiles/libcsma.dir/build

src/csma/CMakeFiles/libcsma.dir/clean:
	cd /home/renan/ns-allinone-3.42/ns-3.42/src/csma && $(CMAKE_COMMAND) -P CMakeFiles/libcsma.dir/cmake_clean.cmake
.PHONY : src/csma/CMakeFiles/libcsma.dir/clean

src/csma/CMakeFiles/libcsma.dir/depend:
	cd /home/renan/ns-allinone-3.42/ns-3.42 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/renan/ns-allinone-3.42/ns-3.42 /home/renan/ns-allinone-3.42/ns-3.42/src/csma /home/renan/ns-allinone-3.42/ns-3.42 /home/renan/ns-allinone-3.42/ns-3.42/src/csma /home/renan/ns-allinone-3.42/ns-3.42/src/csma/CMakeFiles/libcsma.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : src/csma/CMakeFiles/libcsma.dir/depend

