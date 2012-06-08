#!/bin/sh
cp foo.orig.h foo.h
cp foo.orig.cpp foo.cpp
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
make

mkdir -p ../../Build
cd ../../Build/
cmake ../
make
cd -

../../Build/refactorial < test.yml
make
