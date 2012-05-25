#include <string>

namespace A {
  class Foo {
  protected:
    int x;
  public:
    Foo() : x(0) {}
    Foo(int px) : x(px) {}
  };  
};
