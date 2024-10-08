#ifndef _GLOBALHEAP_
#define _GLOBALHEAP_

#define INVALID_BINDLESS_HANDLE 0xFFFFFFFF

#define BUFFER_HEAP_BINDING 0
#define UNIFORM_HEAP_BINDING 1
#define TEXTURE_HEAP_BINDING 2
#define IMAGE_HEAP_BINDING 3
#define TLAS_HEAP_BINDING 4

#ifndef BINDLESS_SET
#define BINDLESS_SET 0
#endif

#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#define DECL_IMAGE_HEAP(SIGNATURE, FORMAT) \
    layout(set=BINDLESS_SET, binding=IMAGE_HEAP_BINDING, FORMAT) \
      SIGNATURE[]

#define DECL_SAMPLER_HEAP(SIGNATURE) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING) \
      SIGNATURE[]

#define SAMPLER2D(NAME) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING) \
      uniform sampler2D NAME[]

#define SAMPLERCUBEARRAY(NAME) \
    layout(set=BINDLESS_SET, binding=TEXTURE_HEAP_BINDING) \
      uniform samplerCubeArray NAME[]

#define IMAGE2D_RW(NAME,FORMAT) \
    layout(set=BINDLESS_SET, binding=IMAGE_HEAP_BINDING, FORMAT) \
      uniform image2D NAME[]
#define IMAGE2D_R(NAME,FORMAT) \
    layout(set=BINDLESS_SET, binding=IMAGE_HEAP_BINDING, FORMAT) \
      readonly image2D NAME[]
#define IMAGE2D_W(NAME) \
    layout(set=BINDLESS_SET,binding=IMAGE_HEAP_BINDING) \
      uniform writeonly image2D NAME[]

#define BUFFER_RW(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      buffer BODY NAME[]
#define BUFFER_R(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      readonly buffer BODY NAME[]
#define BUFFER_W(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      writeonly buffer BODY NAME[]

#define BUFFER_RW_PACKED(NAME,BODY) \
    layout(scalar, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      buffer BODY NAME[]
#define BUFFER_R_PACKED(NAME,BODY) \
    layout(scalar, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      readonly buffer BODY NAME[]
#define BUFFER_W_PACKED(NAME,BODY) \
    layout(scalar, set=BINDLESS_SET, binding=BUFFER_HEAP_BINDING) \
      writeonly buffer BODY NAME[]

// TODO: This garbage would be much simpler with a custom preprocessor to
// resolve bindless syntax sugar

#define DECL_CONSTANTS(TYPE, NAME)                                    \
    BUFFER_R(_##NAME##Heap, _##NAME##_BUFFER{                         \
      TYPE val;                                                       \
    })                             

#define DECL_BUFFER(MODE, TYPE, NAME)                                 \
    BUFFER_##MODE(_##NAME##Heap, _##NAME##_BUFFER{                    \
      TYPE arr[];                                                     \
    })                                                                                     

#define BINDLESS(NAME, HANDLE) \
  uint _##NAME##_handle = HANDLE;

#define GET_CONSTANTS(NAME)                                           \
    _##NAME##Heap[_##NAME##_handle].val 

#define BUFFER_GET(NAME, IDX)                                         \
    _##NAME##Heap[_##NAME##_handle].arr[IDX] 

#define BUFFER_HEAP(TYPE, NAME, START_HANDLE, COUNT_PER_BUFFER)       \
    BUFFER_RW(_##NAME##Heap, _##NAME##_BUFFER{                        \
      TYPE arr[];                                                     \
    });                                                               \
    uint _##NAME##StartHandle() { return START_HANDLE; }              \
    uint _##NAME##CountPerBuffer() { return COUNT_PER_BUFFER; }       

#define BUFFER_HEAP_GET(NAME, IDX)                                             \
    _##NAME##Heap[_##NAME##StartHandle() + (IDX) / _##NAME##CountPerBuffer()]  \
      .arr[(IDX) % _##NAME##CountPerBuffer()]

#define UNIFORM_BUFFER(NAME,BODY) \
    layout(std430, set=BINDLESS_SET, binding=UNIFORM_HEAP_BINDING) \
      uniform BODY NAME[]

#define TLAS(NAME) \
    layout(set=BINDLESS_SET, binding=TLAS_HEAP_BINDING) \
      uniform accelerationStructureEXT NAME[]

#define RESOURCE(NAME,IDX) NAME[IDX]
#define TEXTURE_SAMPLE(NAME,IDX,UV) texture(RESOURCE(NAME,IDX),UV)
#define TEXTURE_LOD_SAMPLE(NAME,IDX,UV,LOD) textureLod(RESOURCE(NAME,IDX),UV,LOD)

#endif // _GLOBALHEAP_