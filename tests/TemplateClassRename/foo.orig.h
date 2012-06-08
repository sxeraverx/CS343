
namespace A {
  template <class T1, class T2>
  class Foo {
  public:
    Foo () : next(0) {}
    Foo (T1 f, T2 s) : first(f), second(s), next(0) {}
    
    ~ Foo() {}
    
    void test(Foo& x) {
      first = x.first;
    }
    
    void test2(Foo<T1, T2>& x) {
      second = x.second;
    }
    
    T1 first;
    T2 second;
    typedef Foo<T1, T2>** PtrPtr;
    Foo<T1, T2>* next;
    PtrPtr ptrToNext;
  };
};
