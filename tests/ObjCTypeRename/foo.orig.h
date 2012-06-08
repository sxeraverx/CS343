#import <Foundation/Foundation.h>

@class Foo;

@protocol FooDelegate <NSObject>
- (void)foo:(Foo *)someFoo didVisitNextFoo:(Foo *)nextFoo;
@end

@interface Foo : NSObject
{
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

@interface Foo (SomeCategory) <FooDelegate>
- (void)blah;
@end

