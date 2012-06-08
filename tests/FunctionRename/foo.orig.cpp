#include "foo.h"

#define w(a, b) a##b
#define ws wasteCycle
#define wws ws
#define call(y, x) do { (void)y; x; } while(0 );

namespace SampleNameSpace {
  int Foo::counter = 0;

  class A : public Foo {
  public:
    void wasteCycle() {
      Foo::wasteCycle();
    }
  };
};

void SampleNameSpace::Foo::doNothing() {
  wasteCycle();
  
  // w(waste, Cycle)();
  // ws();
  // wws();
  
  call(
    1,
    wasteCycle()
  );
  Foo::wasteCycle();
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
  return Foo();
}

Foo test(Foo *a) {
  test();
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
