#import "foo.h"
#import <vector>

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

@implementation Bar
@end

A::Foo test(A::Foo &a) {
  A::Foo b = a;
  return b;
}

int main()
{
    A::Foo aa;
    std::vector<A::Foo> vf;
    
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
