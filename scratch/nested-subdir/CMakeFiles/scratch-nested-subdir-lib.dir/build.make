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
include scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/compiler_depend.make

# Include the progress variables for this target.
include scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/progress.make

# Include the compile flags for this target's objects.
include scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/flags.make

scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o: scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/flags.make
scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o: scratch/nested-subdir/lib/scratch-nested-subdir-library-source.cc
scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o: scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/renan/ns-allinone-3.42/ns-3.42/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o"
	cd /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o -MF CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o.d -o CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o -c /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir/lib/scratch-nested-subdir-library-source.cc

scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.i"
	cd /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir/lib/scratch-nested-subdir-library-source.cc > CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.i

scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.s"
	cd /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir/lib/scratch-nested-subdir-library-source.cc -o CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.s

# Object files for target scratch-nested-subdir-lib
scratch__nested__subdir__lib_OBJECTS = \
"CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o"

# External object files for target scratch-nested-subdir-lib
scratch__nested__subdir__lib_EXTERNAL_OBJECTS =

build/lib/libscratch-nested-subdir-lib.a: scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/lib/scratch-nested-subdir-library-source.cc.o
build/lib/libscratch-nested-subdir-lib.a: scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/build.make
build/lib/libscratch-nested-subdir-lib.a: scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/renan/ns-allinone-3.42/ns-3.42/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library ../../build/lib/libscratch-nested-subdir-lib.a"
	cd /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir && $(CMAKE_COMMAND) -P CMakeFiles/scratch-nested-subdir-lib.dir/cmake_clean_target.cmake
	cd /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/scratch-nested-subdir-lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/build: build/lib/libscratch-nested-subdir-lib.a
.PHONY : scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/build

scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/clean:
	cd /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir && $(CMAKE_COMMAND) -P CMakeFiles/scratch-nested-subdir-lib.dir/cmake_clean.cmake
.PHONY : scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/clean

scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/depend:
	cd /home/renan/ns-allinone-3.42/ns-3.42 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/renan/ns-allinone-3.42/ns-3.42 /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir /home/renan/ns-allinone-3.42/ns-3.42 /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir /home/renan/ns-allinone-3.42/ns-3.42/scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : scratch/nested-subdir/CMakeFiles/scratch-nested-subdir-lib.dir/depend

