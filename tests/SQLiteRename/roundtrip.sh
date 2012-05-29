#!/bin/sh
export FROM_TYPE_QUALIFIED_NAME="sqlite3"
export TO_TYPE_NAME="Foobar"
make -C ../../
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .

../../refactorial shell.c
../../refactorial sqlite3.c

./swap.sh

export FROM_TYPE_QUALIFIED_NAME="Foobar"
export TO_TYPE_NAME="sqlite3"
../../refactorial shell.c
../../refactorial sqlite3.c

diff sqlite3.refactored.h sqlite3.backup.h
diff sqlite3.refactored.c sqlite3.backup.c
diff shell.refactored.c shell.backup.c
rm *.refactored.*
./restore.sh
