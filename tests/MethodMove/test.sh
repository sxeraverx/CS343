#!/bin/sh
export BATCH_MOVE_CLASS_FILTER="class SampleNameSpace::Foo"
export BATCH_MOVE_METHOD_FILTER=".+"
export BATCH_MOVE_HEADER_FILE="foo.h"
export BATCH_MOVE_IMPLEMENTATION_FILE="foo.cpp"
cd ../../
cmake .
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../refactorial foo.cpp
