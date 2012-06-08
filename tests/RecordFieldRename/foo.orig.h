#include <string>

namespace MyNamespace {
  class Foo {
  public:
    int index;
    int a;
    int b;
    int c;
    int m_p;
    int m_q;
    int m_r;
    
    struct {
      int a;
    } m_s;
    
    union {
      int a;
      char b[4];
    } m_u;
    
    Foo() : index(0), a(0), b(0), c(0), m_p(1), m_q(1), m_r(1) {}
    
    int sum();    
  };
  
  void bar(Foo& a, Foo *b);
};
