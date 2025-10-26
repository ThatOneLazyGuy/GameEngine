#if defined(SDL3GPU)
    #if defined(VERTEX)
        #define Sampler 0
        #define Uniform 1
    #elif defined(FRAGMENT)
        #define Sampler 2
        #define Uniform 3
    #endif
#elif defined(DX12)
    #if defined(VERTEX)
        #define Sampler 0
        #define Uniform 1
    #elif defined(FRAGMENT)
        #define Sampler 2
        #define Uniform 3
    #endif
#else//if defined(OPENGL)
    #define Uniform 1
    #define Sampler 0
#endif

#define Bind(bind, type) [vk::binding(bind, type)]