namespace A {
  class Foo {
  public:
    bool x;
    Foo(bool y = true) : x(y) {}
    
    void push(Foo a) {
      x = a.x;      
    }
    
    template<typename T> Foo& test(T a, T b);
  };

  inline template<typename T> Foo& Foo::test(T a, T b) {
      Foo c;
      push(Foo());
      x = (a == b);
      return *this;
    }    

  
  class Bar : public Foo {
  public:
    Bar() : Foo(false) {}
  };
};

