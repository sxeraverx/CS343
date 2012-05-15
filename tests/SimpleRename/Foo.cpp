#include "foo.h"

void Foo::wasteCycle() {
    Foo a(this);
    for (int b = 0; b < 10; b++) {
        Foo c(this);
        Foo d(this);
        c.setX(b);
        d = c;
    }
}

Foo test() {
  return Foo();
}

Foo test(Foo *a) {
  return Foo(a);
}

int main() {
  Foo a = test();
  return a.getX();
}
