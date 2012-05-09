namespace MyNameSpace {
    class Foo {
    private:
        int a;
    public:
        Foo()
        {
            a = 5;
        }
        
        Foo(int x) : a(x) { }
        
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
        
        void inlineTest();
        void inlineTest(int x);
        
        virtual void vtest() {
        }

        virtual void vtest(int x);
        
        const int getA() {
            return a;
        }
    };
    
    inline void Foo::inlineTest() {
        a = 7;
    }
};

inline void MyNameSpace::Foo::inlineTest(int x) {
    a = 8;
}

