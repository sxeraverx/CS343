#include "foo.h"
// #include <vector>
// #include <map>

typedef A::Foo AF;
typedef A::Foo* AFP;

#define Poo A::Foo
#define Blah(x) A::x

A::Foo foo()
{
  return A::Foo(5);
}

A::Foo foo(A::Foo b = A::Foo(6))
{
  return b;
}

A::Foo foo(A::Foo a, A::Foo* b, A::Foo& c, A::Foo***)
{
  return A::Foo(5);
}


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

  // A::Foo &&er = A::Foo();

  A::Foo *f = new A::Foo;
  a = A::Foo(5);
  
  AF g;
  AF* h = &g;
  AFP i = h;
  
  // std::vector<A::Foo> j;
  // std::pair<int, A::Foo> k;
  // std::pair<A::Foo, float> l;
  // std::pair<A::Foo, A::Foo> m;
  // std::pair<A::Foo, std::pair<int, A::Foo> > n;
  
  Blah(Foo) o;
  Poo p;
  Poo* q;  
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
  // std::vector<Foo> j;
  // std::pair<int, Foo> k;
  // std::pair<Foo, float> l;
  // std::pair<Foo, Foo> m;
  // std::pair<Foo, std::pair<int, Foo> > n;  
  ta = Foo(5);
  void *tg = tf;
  Foo *th = (Foo *)tg;
  
  class BBB {
    Foo b;
  };
}
