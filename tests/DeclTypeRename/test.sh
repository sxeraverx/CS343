#!/bin/sh
export FROM_CLASS_NAME="class A::Foo"
export TO_CLASS_NAME="Foobar"
cd ../../
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../refactorial foo.cpp
