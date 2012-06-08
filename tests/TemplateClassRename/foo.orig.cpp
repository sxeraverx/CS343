#include "foo.h"
#include <string>

typedef A::Foo<std::string, std::string> Bar;
 
int main()
{
  A::Foo<int, int> a = A::Foo<int, int>(1, 2);
  A::Foo<int, int>& b = a;
  A::Foo<int, int> *c = new A::Foo<int, int>(1, 2);
  delete c;
  
  A::Foo<std::string, std::string> d;
  Bar e = d;
  
  return 0;
}
