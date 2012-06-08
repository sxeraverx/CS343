#!/bin/sh
cp foo.orig.h foo.h
cp foo.orig.cpp foo.cpp
clang -c foo.cpp
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .

mkdir -p ../../Build
cd ../../Build/
cmake ../
make
cd -
../../Build/refactorial < refax.yml
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
