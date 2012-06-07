#!/bin/sh
cp foo.orig.cpp foo.cpp
cp foo.orig.h foo.h
mkdir -p ../../Build/
(cd ../../Build; cmake ../; make)
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON


# remember, in YAML, * has meaning, so have to quote things
../../Build/refactorial <<EOF
---
Transforms:
  RecordFieldRename:
    Ignore:
      - /usr/include/.*
    Fields:
      - MyNamespace::Foo::index: i
      - MyNamespace::Foo::m_(.+): _\\1
EOF
