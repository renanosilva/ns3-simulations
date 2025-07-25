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
include src/uan/CMakeFiles/libuan.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/uan/CMakeFiles/libuan.dir/compiler_depend.make

# Include the progress variables for this target.
include src/uan/CMakeFiles/libuan.dir/progress.make

# Include the compile flags for this target's objects.
include src/uan/CMakeFiles/libuan.dir/flags.make

# Object files for target libuan
libuan_OBJECTS =

# External object files for target libuan
libuan_EXTERNAL_OBJECTS = \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/helper/acoustic-modem-energy-model-helper.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/helper/uan-helper.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/acoustic-modem-energy-model.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-channel.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-header-common.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-header-rc.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-aloha.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-cw.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-rc-gw.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-rc.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-net-device.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-noise-model-default.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-noise-model.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-phy-dual.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-phy-gen.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-phy.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-prop-model-ideal.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-prop-model-thorp.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-prop-model.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-transducer-hd.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-transducer.cc.o" \
"/home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan-obj.dir/model/uan-tx-mode.cc.o"

build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/helper/acoustic-modem-energy-model-helper.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/helper/uan-helper.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/acoustic-modem-energy-model.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-channel.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-header-common.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-header-rc.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-aloha.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-cw.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-rc-gw.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac-rc.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-mac.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-net-device.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-noise-model-default.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-noise-model.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-phy-dual.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-phy-gen.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-phy.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-prop-model-ideal.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-prop-model-thorp.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-prop-model.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-transducer-hd.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-transducer.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan-obj.dir/model/uan-tx-mode.cc.o
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan.dir/build.make
build/lib/libns3.42-uan-default.so: src/uan/CMakeFiles/libuan.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/renan/ns-allinone-3.42/ns-3.42/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking CXX shared library ../../build/lib/libns3.42-uan-default.so"
	cd /home/renan/ns-allinone-3.42/ns-3.42/src/uan && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/libuan.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/uan/CMakeFiles/libuan.dir/build: build/lib/libns3.42-uan-default.so
.PHONY : src/uan/CMakeFiles/libuan.dir/build

src/uan/CMakeFiles/libuan.dir/clean:
	cd /home/renan/ns-allinone-3.42/ns-3.42/src/uan && $(CMAKE_COMMAND) -P CMakeFiles/libuan.dir/cmake_clean.cmake
.PHONY : src/uan/CMakeFiles/libuan.dir/clean

src/uan/CMakeFiles/libuan.dir/depend:
	cd /home/renan/ns-allinone-3.42/ns-3.42 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/renan/ns-allinone-3.42/ns-3.42 /home/renan/ns-allinone-3.42/ns-3.42/src/uan /home/renan/ns-allinone-3.42/ns-3.42 /home/renan/ns-allinone-3.42/ns-3.42/src/uan /home/renan/ns-allinone-3.42/ns-3.42/src/uan/CMakeFiles/libuan.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : src/uan/CMakeFiles/libuan.dir/depend

