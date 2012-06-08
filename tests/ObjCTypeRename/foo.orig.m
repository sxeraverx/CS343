#import "foo.h"

@implementation Foo

@synthesize name;
@synthesize next;
@synthesize delegate;

- (void)dealloc
{
    next = nil;
    [name release];
    [super dealloc];
}

- (id)initWithFoo:(Foo *)someFoo
{
    self = [super init];
    if (self) {
        next = someFoo;
    }
    return self;
}

- (id)initWithName:(NSString *)someName
{
    self = [super init];
    if (self) {
        name = [someName copy];
    }
    return self;
}

- (Foo *)tail
{
    Foo *t = next;
    while (t && t.next) {
        [delegate foo:self didVisitNextFoo:t];
        t = t.next;
    }
    return t;
}
@end

@implementation Foo (SomeCategory)
- (void)blah
{
}

- (void)foo:(Foo *)someFoo didVisitNextFoo:(Foo *)nextFoo
{
  if ([Foo conformsToProtocol:@protocol(FooDelegate)]) {
    const char *t = @encode(Foo);
  }
}
@end

@implementation Bar
@end


int main()
{
    @autoreleasepool {
        Foo *a = [[[Foo alloc] initWithName:@"Foo"] autorelease];
        Foo *b = [[[Foo alloc] initWithFoo:a] autorelease];
        Foo *c = b.next;
        Foo *d = [b tail];
        NSLog(@"%d", c == d);
        NSLog(@"%@", a.name);
    }
    return 0;
}
