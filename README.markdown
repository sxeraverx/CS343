
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
    cmake ../llvm
    make
    sudo make install
    
You probably noticed that we don't use `-std=c++0x` in `CXXFLAGS`. Parts of LLVM won't build with C++11 enabled.


## Generating `compile_commands.json`

Needed for your CMake projects.

See http://scitools.com/blog/2012/04/cmake-and-understand.html for how to generate that file. Or you can do this:

    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON <your build dir>


## Testing ClassRenameTransform

Use `test.sh`:

    cd tests/SimpleRename
    ./test.sh

Make sure you rebuild the main project (by regenerating the Makefile then make) first before testing.
