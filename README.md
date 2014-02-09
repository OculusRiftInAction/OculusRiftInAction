OculusRiftExamples
==================

Example code for using the Oculus Rift

# Notes

This example code is designed to use static linking throughout.  This means static linking against the C/C++ runtimes 
on windows (to avoid issues with missing Visual Studio redistributables) and static linking against all of the 
included submodules.  Dynamic linking is only used for system libraries (other than the C++ runtime on Windows) and 
for the resource DLL on windows.  In order to avoid having each executable contain a distinct copy of the (currently 
30MB) resource files, a single DLL is created to hold them, and the common library functionality has utility methods 
to load them.

# Instructions for building (all platforms)

	git clone https://github.com/OculusCommunitySDK/OculusRiftExamples.git OculusRiftExamples
	cd OculusRiftExamples
	git submodule init
	git submodule update
	mkdir build && cd build
	cmake .. [-G <your preferred toolset>]

The "Unix Makefiles" line above needs to be modified depending on what platform you're 

# CMake details

CMake will create project files for a given toolset.  What toolset you use depends on your platform, and there's usually a 
default.  On a Windows machine, it defaults to Visual Studio, although if you have more than one version installed it may 
require you to specify which one you want.  You can specify a specific platform using -G.  

    cmake -h

Will show you a list of supported generators on your platform.

# Supported platforms

## On Windows 8 with Visual Studio 2013 Desktop Edition

The following generators have all been found to work

Suitable generators

* "Visual Studio 11"
* "Visual Studio 11 Win64"
* "Visual Studio 12"
* "Visual Studio 12 Win64"

It's worthwhile to note that CMake may not show the Win64 options in the list of generators, even if they're supported.
Win64 refers to the creation of projects that produce 64 bit binaries, NOT the version of Visual Studio itself. 

The libraries needed to build the core examples are included and built as part of the project, but some examples will 
only be built if additional libraries are present, such as OpenCV.  If you want to build these, it's important to make 
sure you match up the 32/64 bit nature of the project you create with the installed version of OpenCV, etc.  

## On OSX 10.8.5 with XCode 5.0

* "Xcode"

## On Ubuntu 13.04 with using Eclipse 4.3 (Kepler) 

Suitable generators

* "Eclipse CDT4 - Unix Makefiles"
* "Eclipse CDT4 - Ninja"
* "Unix Makefiles"
* "Ninja"

Note that the CMake Eclipse generators will complain about a build directory that is a child of the source directory, 
but I have not experienced any issues with this. 

