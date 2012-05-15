#include "foo.h"

using namespace SampleNameSpace;

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

Foo globalFoo;

int main() {
  class Bar {
  public:
    Foo a;
    void test() {
      a.setX(10);
    }
  };

  Foo a = test();
  Bar b;
  b.test();
  return a.getX();
}
