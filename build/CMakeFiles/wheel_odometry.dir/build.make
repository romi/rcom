# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


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
CMAKE_SOURCE_DIR = /home/issam/rcom

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/issam/rcom/build

# Include any dependencies generated for this target.
include CMakeFiles/wheel_odometry.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/wheel_odometry.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/wheel_odometry.dir/flags.make

../examples/wheel_odometry/wheel_odometry_main.c: rcgen
../examples/wheel_odometry/wheel_odometry_main.c: ../examples/wheel_odometry/wheel_odometry.json
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/issam/rcom/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating ../examples/wheel_odometry/wheel_odometry_main.c"
	./rcgen /home/issam/rcom/examples/wheel_odometry/wheel_odometry.json /home/issam/rcom/examples/wheel_odometry/wheel_odometry_main.c

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o: CMakeFiles/wheel_odometry.dir/flags.make
CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o: ../examples/wheel_odometry/wheel_odometry.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/issam/rcom/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o   -c /home/issam/rcom/examples/wheel_odometry/wheel_odometry.c

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/issam/rcom/examples/wheel_odometry/wheel_odometry.c > CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.i

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/issam/rcom/examples/wheel_odometry/wheel_odometry.c -o CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.s

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.requires:

.PHONY : CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.requires

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.provides: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.requires
	$(MAKE) -f CMakeFiles/wheel_odometry.dir/build.make CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.provides.build
.PHONY : CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.provides

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.provides.build: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o


CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o: CMakeFiles/wheel_odometry.dir/flags.make
CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o: ../examples/wheel_odometry/wheel_odometry_main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/issam/rcom/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o   -c /home/issam/rcom/examples/wheel_odometry/wheel_odometry_main.c

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/issam/rcom/examples/wheel_odometry/wheel_odometry_main.c > CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.i

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/issam/rcom/examples/wheel_odometry/wheel_odometry_main.c -o CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.s

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.requires:

.PHONY : CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.requires

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.provides: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.requires
	$(MAKE) -f CMakeFiles/wheel_odometry.dir/build.make CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.provides.build
.PHONY : CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.provides

CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.provides.build: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o


# Object files for target wheel_odometry
wheel_odometry_OBJECTS = \
"CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o" \
"CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o"

# External object files for target wheel_odometry
wheel_odometry_EXTERNAL_OBJECTS =

wheel_odometry: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o
wheel_odometry: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o
wheel_odometry: CMakeFiles/wheel_odometry.dir/build.make
wheel_odometry: librcom.so
wheel_odometry: CMakeFiles/wheel_odometry.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/issam/rcom/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking C executable wheel_odometry"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/wheel_odometry.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/wheel_odometry.dir/build: wheel_odometry

.PHONY : CMakeFiles/wheel_odometry.dir/build

CMakeFiles/wheel_odometry.dir/requires: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry.c.o.requires
CMakeFiles/wheel_odometry.dir/requires: CMakeFiles/wheel_odometry.dir/examples/wheel_odometry/wheel_odometry_main.c.o.requires

.PHONY : CMakeFiles/wheel_odometry.dir/requires

CMakeFiles/wheel_odometry.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/wheel_odometry.dir/cmake_clean.cmake
.PHONY : CMakeFiles/wheel_odometry.dir/clean

CMakeFiles/wheel_odometry.dir/depend: ../examples/wheel_odometry/wheel_odometry_main.c
	cd /home/issam/rcom/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/issam/rcom /home/issam/rcom /home/issam/rcom/build /home/issam/rcom/build /home/issam/rcom/build/CMakeFiles/wheel_odometry.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/wheel_odometry.dir/depend

