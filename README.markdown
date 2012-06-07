
## Build LLVM and Clang with CMake

This project uses CMake, and it expects to find its dependencies via other CMake files.

Because of this, you also need to build LLVM and Clang with CMake. Here's what we use:

    # suppose you checked out llvm in ./llvm
    mkdir build-llvm
    cd build-llvm    
    cmake ../llvm
    make
    sudo make install

If you're on Mac OS X, make sure you read the next section.


## Prerequisites for Mac OS X

Since we use C++11, make sure your LLVM and Clang are built with the latest libc++ on Mac OS X.

Go to http://libcxx.llvm.org/ to check out a copy of the latest libc++.

Then you need to build LLVW and Clang with the overridden compiler and linker settings. Here's what I do before running CMake:

    # suppose you checked out llvm in ./llvm
    mkdir build-llvm
    cd build-llvm    
    export CXXFLAGS=-stdlib=libc++
    export LDFLAGS=-stdlib=libc++
    cmake -DCMAKE_BUILD_TYPE:STRING=Release \
        -DCMAKE_CXX_FLAGS:STRING=-stdlib=libc++ \
        -DCMAKE_SHARED_LINKER_FLAGS:STRING=-stdlib=libc++ .
    cmake ../llvm
    make
    sudo make install
    
You probably noticed that we don't use `-std=c++0x` in `CXXFLAGS`. Parts of LLVM won't build with C++11 enabled.

We also use yaml-cpp now. Check out the source at: http://code.google.com/p/yaml-cpp/source/checkout

yaml-cpp uses Boost. You can use the one installed by MacPorts or any other package manager.

Note that yaml-cpp **must be build with the Clang compiler you've built**! This is because the compiler that comes with Xcode has a bug (http://llvm.org/bugs/show_bug.cgi?id=13003) that cause it to fail compiling yaml-cpp.

Insert the following lines to CMakeLists.txt in the yaml-cpp project root, right after the line `include(CheckCXXCompilerFlag)`:

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ --std=c++0x -I/opt/local/include")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    SET(CMAKE_C_COMPILER /usr/local/bin/clang)
    SET(CMAKE_CXX_COMPILER /usr/local/bin/clang++)

And build yaml-cpp. Now you're all set.





## Generating `compile_commands.json`

Needed for your CMake projects.

See http://scitools.com/blog/2012/04/cmake-and-understand.html for how to generate that file. Or you can do this:

    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON <your build dir>


## Testing ClassRenameTransform

Use `test.sh`:

    cd tests/SimpleRename
    ./test.sh

Make sure you rebuild the main project (by regenerating the Makefile then make) first before testing.


## MethodMoveTransform

MethodMoveTransform moves all method bodies in the designated class to the
implementation file.

*   Input: `BATCH_MOVE_CLASS_NAME`

To test MethodMoveTransform:

Use `test.sh`:

    cd tests/MethodMove
    ./test.sh

Make sure you rebuild the main project (by regenerating the Makefile then make) first before testing.

