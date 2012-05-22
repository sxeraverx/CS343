namespace B {};
namespace C { 
    void blah();
};



namespace A {
    using namespace B;
    using namespace C;

namespace MyNameSpace {
    using namespace C;
    
	class Foo {
	protected:
        class Inner {};
	private:
		int a;
		int b;
	public:
		Foo() {
            blah();
			a = 5;
			b = 4;
		}
        
		Foo(int x) : a(x) { b = 3; }
		
		Foo(int* x) : a(*x), b(*x + 1) {}
        
		Foo(int x, int y);
        
		virtual ~Foo() {
			a = 0;
		}
        
		virtual void pure() = 0;
		virtual void pure(int x) = 0;
		virtual int pureWithReturn() = 0;
        
		void test() {
			a = 6;
		}
        
		void test(int x);
		void test(int x, int y);
        
		int testConstFunc(int x) const {
			return a + x;
		}

		int testConstFunc(int x, int y, int z = 5) const {
			return a + x + y + z;
		}

        
		void inlineTest();
		void inlineTest(int x);
        
		virtual void vtest() {
		}

		virtual void vtest(int x);
        
		const int getA() {
			return a;
		}
        
		static void StaticMethod() {
		}
	};
    
	class Bar : Foo {
	public:
		virtual void vtest() {
		}        
	};
    
	inline void Foo::inlineTest() {
		a = 7;
	}
};

inline void MyNameSpace::Foo::inlineTest(int x) {
	a = 8;
}


};
