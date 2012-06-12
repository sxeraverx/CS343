
# Refactorial

Refactoiral is a Clang-based refactoring tool for C, C++, Objective-C, and Objective-C++.

## Install

There are two ways to install Refactorial:

*   Download a pre-built binary. Currently on Mac OS X only. Get it here:
    https://github.com/lukhnos/refactorial/downloads
*   Build on your own. See below.

## Building Refactorial

We have built Refactorial on both Linux and Mac OS X.

You need the following dependencies:

*   LLVM and Clang
*   yaml-cpp
*   Boost (needed by yaml-cpp)
*   pcre

For LLVM and Clang, you need to build them from the latest source. *Using the
built-in one that comes with Xcode 4.2 won't work*.

Use the latest yaml-cpp from source (most binary distributions won't work).
Get it from http://code.google.com/p/yaml-cpp/source/checkout

For Boost and pcre, you can install them in whatever way you like. Latest
binary distribution versions will do.

To build Refactorial, you need to use the latest Clang compiler. yaml-cpp uses
templates extensively and some instantiations can't be compiled using older
Clang versions. gcc should also work.

Refactorial is written in C++11, although Clang will happily compile it
even without turning on C++11 (we don't use that much other then `auto`).
If you want to build Refactorial with C++11 on (`--std=c++0x`), you'll
probably also need to build LLVM, Clang, yaml-cpp and pcre with the same
option. Which can be a pain due to linkage issues.

To build Refactorial, do this:

    cmake .
    make

If you're on OS X, the default is to use the Clang installed in
`/usr/local`. This assumes you have built your own Clang (which is what we
do).

### Building Refactorial and Its Dependencies with C++11 Enabled

You can safely skip this section.

If you really want to build Refactorial with C++11 enabled, you will also need
to install libc++ (rev 157242, recent versions won't work). Build all
dependencies (all of them use CMake) with:

    cmake -DCMAKE_BUILD_TYPE:STRING=Release \
        -DCMAKE_CXX_FLAGS:STRING=-stdlib=libc++ \
        -DCMAKE_SHARED_LINKER_FLAGS:STRING=-stdlib=libc++ \
        .
    make
    sudo make install

Then build Refactorial with:

    cmake -DCMAKE_BUILD_TYPE:STRING=Release \
        -DCMAKE_CXX_FLAGS:STRING="-stdlib=libc++ --std=c++0x" \
        -DCMAKE_SHARED_LINKER_FLAGS:STRING=-stdlib=libc++ \
        .
    make


## Using Refactorial

### Generating `compile_commands.json`

Clang-based tools (to be specific, those that use LibTooling) use the
"compilation database" to know which source files to parse with which
compiler options. It can be seen as a condensed Makefile.

CMake, which is a popular GNU Autotools replacement ("`./configure; make`"),
will happily generate the compilation database for your CMake project:

    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON <your build dir>
    
LLVM incidentally also uses CMake, and so do many popular open source projects.

There is currently no way to generate a compilation database out of a
Makefile or an IDE project (e.g. Microsoft `.vcproj` or Xcode's `.xcodeproj`).
That's something that we need to work on.


## Transforms Provided

*   Accessor: Synthesize getters and setters for designated member variables
*   MethodMove: Move inlined member function bodies to the implementation file
*   ExtractParameter: promote a function variable to a parameter to that function
*   TypeRename: Rename types, including tag types (enum, struct, union, class), template classes, Objective-C types (class and protocol), typedefs and even bulit-in types (e.g. `unsigned` to `uint32_t`)
*   RecordFieldRename: Rename record fields, including C++ member variables
*   FunctionRename: Rename functions, including C++ member functions

Documentation upcoming. Before that, take a look at our test cases in
`tests/`. You can get an idea what each source transform does and which
parameters they take.


## Copyright License

Copyright © 2012 Lukhnos Liu and Thomas Minor.

MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

