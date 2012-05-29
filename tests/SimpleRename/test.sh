#!/bin/sh
export FROM_CLASS_NAME="class SampleNameSpace::Foo"
export TO_CLASS_NAME="Foobar"
cd ../../
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../Build/refactorial Foo.cpp
