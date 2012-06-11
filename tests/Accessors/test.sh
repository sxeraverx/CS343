[ -e foo.cpp.orig ] && mv foo.cpp.orig foo.cpp
[ -e foo.h.orig ] && mv foo.h.orig foo.h
mkdir -p ../../Build/
(cd ../../Build; cmake ../; make)
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
../../Build/refactorial <<EOF
---
Transforms:
  Accessors:
    - Foo::x
EOF
