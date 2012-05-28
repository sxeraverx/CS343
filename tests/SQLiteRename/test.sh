#!/bin/sh
export FROM_TYPE_QUALIFIED_NAME="sqlite3"
export TO_TYPE_NAME="Foobar"
cd ../../
make
cd -
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
../../refactorial shell.c
../../refactorial sqlite3.c

