#!/bin/sh

# build the refactoring tool
mkdir -p ../../Build
cd ../../Build/
cmake ../
make
cd -

# fetch SQLite3 if it's not in place
if [ ! -f sqlite3.orig.c ]
then
  B=sqlite-amalgamation-3071201
  Z=$B.zip
  T=$RANDOM
  echo Using temp dir $PWD/$T, downloading http://www.sqlite.org/$Z
  mkdir -p $T
  cd $T
  curl -O http://www.sqlite.org/$Z
  unzip $Z
  mv $B/sqlite3.c ../sqlite3.orig.c
  mv $B/sqlite3.h ../sqlite3.orig.h
  mv $B/sqlite3ext.h ../sqlite3ext.orig.h
  mv $B/shell.c ../shell.orig.c
  cd -
  rm -rf $T
fi

cp sqlite3.orig.c sqlite3.c
cp sqlite3.orig.h sqlite3.h
cp sqlite3ext.orig.h sqlite3ext.h
cp shell.orig.c shell.c

# first, make sure we have a working SQLite3
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS:STRING=ON .
make
./sqlite3 test.db "select * from test"


# now, our first round of rename
../../Build/refactorial < forward.yml
make
./sqlite3 test.db "select * from test"

# then, the inverse
../../Build/refactorial < inverse.yml
make
./sqlite3 test.db "select * from test"
diff sqlite3.orig.c sqlite3.c
diff sqlite3.orig.h sqlite3.h
diff sqlite3ext.orig.h sqlite3ext.h
diff shell.orig.c shell.c
