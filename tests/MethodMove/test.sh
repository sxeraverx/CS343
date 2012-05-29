#!/bin/sh
git checkout foo.cpp
git checkout bar.cpp
git checkout foo.h
mkdir -p ../../Build/
cd ../../Build/
cmake .
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../Build/refactorial < refax.yml
