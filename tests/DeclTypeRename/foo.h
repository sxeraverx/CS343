// #include <string>

namespace A {
  enum       Test {
    B, C
  };
  
  struct O {
    int x;
  };
    
  class P {};
  class X : virtual P {};
  class Y : virtual P {};
  class Foo : public X, virtual Y {
  protected:
    int x;
  public:
    Foo() : x(0) {}
    Foo(int px) : x(px) {}
  };  
  
  class R : virtual public Foo {    
  };
  
  class S : virtual public Foo {    
  };
  
  class T : public R, S {    
  };
  
  class U : public Foo {    
  };
  
  class V : public Foo {    
  };
  
  class W : public U, V {    
  };
};
