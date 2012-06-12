
namespace A {  
  class Foo {
  public:
    int x;
    template<typename T> T as() {
      T a;
      return a;
    }    

    template<typename T1, typename T2> T1 as2() {
      T2 b;
      T1 a;
      return a;
    }    

  };
};
