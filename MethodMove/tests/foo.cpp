#include "foo.h"

namespace MyNameSpace {
    Foo::Foo(int x, int y) {
        a = x + y;
    }
};

void MyNameSpace::Foo::test(int x) {
    a = x * 2;
}

using namespace MyNameSpace;

void Foo::test(int x, int y) {
    a = x + y;
}
