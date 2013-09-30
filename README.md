OculusRiftExamples
==================

Example code for using the Oculus Rift

# Instructions for building (on Linux)

	git clone https://github.com/OculusCommunitySDK/OculusRiftExamples.git OculusRiftExamples
	cd OculusRiftExamples
	git submodule init
	git submodule update
	mkdir build && cd build
	cmake .. -G "Unix Makefiles"
	make -j 4
	./source/Example00/Example00

# Other platforms

The steps are the same until the CMake invokation.  Instead of the "Unix Makefiles" generator, you need to specify the 
generator for whatever build tool you're using.  Call CMake with your desired target project type, and then use the
build tool with the created project files.

The code has been tested using the following CMake generators

## On Ubuntu 13.04 with using Eclipse 4.3 (Kepler) 

* "Eclipse CDT4 - Unix Makefiles"
* "Eclipse CDT4 - Ninja"
* "Unix Makefiles"
* "Ninja"

Note that the CMake Eclipse generators will complain about a build directory that is a child of the source directory, 
but I have not experienced any issues with this. 

## On Windows 8 with Visual Studio 2012 Desktop Edition

* "Visual Studio 11 Win64"

For this I was using the CMake GUI rather than the command line, but it shouldn't be an issue.

## On OSX 10.8.5 with XCode 5.0

* "Xcode"

 