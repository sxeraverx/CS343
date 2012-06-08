#!/bin/sh
cp test.orig.cpp test.cpp
cp utf8.orig.h utf8.h
cp utf8/checked.orig.h utf8/checked.h
cp utf8/core.orig.h utf8/core.h
cp utf8/unchecked.orig.h utf8/unchecked.h
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
make

mkdir -p ../../Build
cd ../../Build/
cmake ../
make
cd -

../../Build/refactorial < test.yml
touch test.cpp
make
