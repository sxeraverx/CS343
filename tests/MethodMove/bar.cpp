#include "foo.h"


namespace A {
namespace MyNameSpace {
	Foo::Baz(int x, int y) {
		a = x + y;
	}
};
};

void A::MyNameSpace::Foo::test(int x) {
	a = x * 2;
}

using namespace A::MyNameSpace;

void Foo::test(int x, int y) {
 a = x + y;
}
