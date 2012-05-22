#!/bin/sh
rm *.refactored.*
export BATCH_MOVE_CLASS_NAME="A::MyNameSpace::Foo"
cd ../../
cmake .
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../refactorial foo.cpp
