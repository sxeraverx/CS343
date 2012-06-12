#include <vector>
#include "foo.h"

using namespace A;
using namespace std;

int main()
{
  A::Foo a;
  vector<vector<A::Foo *> > c2;
  c2 = a.as<vector<vector<A::Foo *> > >();
  c2 = a.as2< vector<vector<A::Foo *> >,  vector<A::Foo *> >();
}
