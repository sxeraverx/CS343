#include "foo.h"

typedef A::Foo AF;
typedef A::Foo* AFP;

int main()
{
  A::Foo a;
  A::Foo *b = &a;
  A::Foo* bb = &a;
  A::Foo*bbb = &a;
  A::Foo  *  bbbb = &a;

  A::Foo **c = &b;
  const A::Foo *d = b;
  A::Foo &e = a;

  // A::Foo &&er = a;

  A::Foo *f = new A::Foo;
  a = A::Foo(5);
  
  AF g;
  AF* h = &g;
  AFP i = h;
}

using namespace A;

void test()
{
  Foo ta;
  Foo *tb = &ta;
  Foo **tc = &tb;
  const Foo *td = tb;
  Foo &te = ta;
  Foo *tf = new Foo;
  ta = Foo(5);  
}
