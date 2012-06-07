cp foo.cpp.orig foo.cpp
cp foo.h.orig foo.h
mkdir -p ../../Build/
(cd ../../Build; cmake ../; make)
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
../../Build/refactorial <<EOF
---
Transforms:
  Accessors:
    - Foo::x
EOF
