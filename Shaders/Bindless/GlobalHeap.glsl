#ifndef _GLOBALHEAP_
#define _GLOBALHEAP_

#define BUFFER_HEAP_BINDING 0
#define UNIFORM_HEAP_BINDING 1
#define TEXTURE_HEAP_BINDING 2

#ifndef BINDLESS_SET
#define BINDLESS_SET 0
#endif

#extension GL_EXT_nonuniform_qualifier : enable

#define SAMPLER2D(NAME) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING) \
      uniform sampler2D NAME[]

#define SAMPLERCUBEARRAY(NAME) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING) \
      uniform samplerCubeArray NAME[]

#define IMAGE2D_RW(NAME,FORMAT) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING, FORMAT) \
      image2D NAME[]
#define IMAGE2D_R(NAME,FORMAT) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING, FORMAT) \
      readonly image2D NAME[]
#define IMAGE2D_W(NAME) \
    layout(set=BINDLESS_SET,binding=TEXTURE_HEAP_BINDING) \
      writeonly image2D NAME[]

#define BUFFER_RW(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      buffer BODY NAME[]
#define BUFFER_R(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      readonly buffer BODY NAME[]
#define BUFFER_W(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      writeonly buffer BODY NAME[]

#define UNIFORM_BUFFER(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=UNIFORM_HEAP_BINDING) \
      uniform BODY NAME[]

#define RESOURCE(NAME,IDX) NAME[IDX]
#define TEXTURE_SAMPLE(NAME,IDX,UV) texture(RESOURCE(NAME,IDX),UV)
#define TEXTURE_LOD_SAMPLE(NAME,IDX,UV,LOD) textureLod(RESOURCE(NAME,IDX),UV,LOD)

#endif // _GLOBALHEAP_