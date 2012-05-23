#include "foo.h"
// #include <iostream>
// #include <vector>

int C::Nana::add(int x) {
  A::Foo p(x);
  return f.add(p.get());
}

#define FX(x)   A::Foo x;

// typedef std::vector<const A::Foo*> fvcp;
// typedef std::vector<A::Foo*> fvp;

int main()
{
  C::Nana n;
  // std::cout << n.add(10);
  
  A::Foo *f = new A::Foo();

  A::Foo a;
  // std::vector<A::Foo> vf;
  // vf.push_back(a);
  
  FX(b);
  // vf.push_back(b);
  
  // fvcp c;
  // c.push_back(f);
  // 
  // fvp d;
  // d.push_back(f);
  
  delete f;
  
  return 0;
}

