FUSION Metrics Programs
=======================

The directory structure of this project:

  project/ ............... The project's root directory
    src/   ............... Source files for the programs and shared code
    test/  ............... Unit tests
    build/ ............... Where the programs and tests are built


Prerequisites for Building the Programs & Tests
-----------------------------------------------

  *  CMake - cross-platform build system

     It produces project files for the developer tools native to your
     platform.  It's available in binary form for various platforms from
	 http://www.cmake.org/.  Download and install the executable for your
	 platform.

  *  C++ compiler

     On OS X, you can use Apple's Xcode IDE.
     On Windows, you can use Visual C++ Express Edition.

  *  Boost C++ libraries, version 1.39

     They are available at http://www.boost.org/.  They come pre-built and
     installed on popular Unix and Linux systems.  You'll have to download
     them if you're on OS X or Windows.  The Boost "Getting Started Guide"
     describes how to install the libraries.
     
     Note: Some of the Boost libraries used by the programs and its tests
     have binaries (i.e., they are not header-only libraries):

       Library binaries       Used by (P=programs, T=tests)
       ----------------       -------
       Boost.Test                 T


Building the Unit Tests
-----------------------

See the file "test/README.txt for details.
