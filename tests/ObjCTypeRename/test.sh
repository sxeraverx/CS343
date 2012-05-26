#!/bin/sh
export FROM_TYPE_QUALIFIED_NAME="Foo"
export TO_TYPE_NAME="Foobar"
cd ../../
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../refactorial foo.m

