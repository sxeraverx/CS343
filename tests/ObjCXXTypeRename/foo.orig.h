#import <Foundation/Foundation.h>

namespace CXX {
  class Foo {
  public:
    int x;
  };
};

@class Foo;

@protocol FooDelegate <NSObject>
- (void)foo:(Foo *)someFoo didVisitNextFoo:(Foo *)nextFoo;
@end

@interface Foo : NSObject
{
  CXX::Foo *f;
  Foo *next;
  NSString *name;
  id<FooDelegate> delegate;
}
- (id)initWithFoo:(Foo *)someFoo;
- (id)initWithName:(NSString *)someName;
- (Foo *)tail;
@property (retain, nonatomic) NSString *name;
@property (assign, nonatomic) Foo *next;
@property (assign, nonatomic) id<FooDelegate> delegate;
@end

@interface Bar : Foo
@end

namespace A {
  class X {
    
  };
  
  class Foo : public X {
  protected:
    int x;
  public:
    Foo() : x(0) {}
    Foo(int px) : x(px) {}
    
    Foo getZeroFoo() {
      return Foo(0);
    }
    
    Foo *createNewFoo() {
      return new Foo(0);
    }
    
    int add(const Foo& f) {
      return x + f.x;
    }
    
    int sub(Foo *f) {
      return x - f->x;
    }
    
    int get() {
      return x;
    }
  };  
};
