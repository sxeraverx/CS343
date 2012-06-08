#include "foo.h"

int MyNamespace::Foo::sum()
{
  return index + a + b + c + m_p + m_q + m_r;
}

void MyNamespace::bar(MyNamespace::Foo& a, MyNamespace::Foo *b) {
  struct t {
    MyNamespace::Foo *a;
  } x;
      
  x.a = b;
  Foo *c = x.a;
  c->m_u.a = 0;
  
  b->m_u.a = 0;
  if (b->m_u.a) {
    
  }
  
  a.m_u.a = b->index;
  b->m_u.b[0] = a.m_u.b[1];
  b->m_s.a = a.index;
  a.m_s.a = a.index;
  a.index = b->m_p;
}  

int main()
{
  return 0;
}
