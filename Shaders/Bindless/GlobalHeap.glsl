#ifndef _GLOBALHEAP_
#define _GLOBALHEAP_

#define BUFFER_HEAP_BINDING 0
#define TEXTURE_HEAP_BINDING 1

#ifndef BINDLESS_SET
#define BINDLESS_SET 0
#endif

#define SAMPLER2D(NAME)( \
    layout(location=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING) \
      uniform sampler2D NAME[])

#define IMAGE2D_RW(NAME,FORMAT)( \
    layout(location=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING, FORMAT) \
      image2D NAME[])
#define IMAGE2D_R(NAME,FORMAT)( \
    layout(location=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING, FORMAT) \
      readonly image2D NAME[])
#define IMAGE2D_W(NAME)( \
    layout(location=BINDLESS_SET,binding=TEXTURE_HEAP_BINDING) \
      writeonly image2D NAME[])

#define BUFFER_RW(NAME,BODY)( \
    layout(std430, location=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      buffer struct BODY NAME[])
#define BUFFER_R(NAME,BODY)( \
    layout(std430, location=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      readonly buffer struct BODY NAME[])
#define BUFFER_W(NAME,BODY)( \
    layout(std430,location=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      writeonly buffer struct BODY NAME[])

#define UNIFORM_BUFFER(NAME,BODY)( \
    layout(std430, location=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      uniform struct BODY NAME[])

#define RESOURCE(NAME,IDX)(NAME[IDX])
#define TEXTURE_SAMPLE(NAME,IDX,UV)(texture(RESOURCE(NAME,IDX),UV))

#endif // _GLOBALHEAP_