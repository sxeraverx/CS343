#!/bin/sh
git checkout Foo.cpp
git checkout Foo.h
mkdir -p ../../Build
cd ../../Build/
cmake ../
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../Build/refactorial < refax.yml
