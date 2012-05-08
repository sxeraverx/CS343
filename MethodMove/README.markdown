Currently MethodMove only rewrites included headers so that inline method
bodies are removed. Initializer in constructor is not yet correctly removed.

To test:

    make
    ./MethodMove tests/foo.cpp
    
And the rewritten header will be saved to `foo_out.h`.
