#include "foo.h"

namespace SampleNameSpace {
  int Foo::counter = 0;
};

void SampleNameSpace::Foo::doNothing() {
}

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
  Foo a = Foo::Foo();
  return Foo();
}

Foo test(Foo *a) {
  return Foo(a);
}

::SampleNameSpace::Foo globalFoo;

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