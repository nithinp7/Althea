#ifndef _HAIRRESOURCES_
#define _HAIRRESOURCES_

#define IS_SHADER
#include "HairCommon.glsl"
#include <Bindless/GlobalHeap.glsl>
#include <Global/GlobalUniforms.glsl>
#include <Global/GlobalResources.glsl>
#include <Misc/Input.glsl>

#extension GL_EXT_nonuniform_qualifier : enable

layout(push_constant) uniform _PushConstants{
  PushConstants pushConstants;
};

#define resources RESOURCE(globalResources, pushConstants.globalResourcesHandle)
#define globals RESOURCE(globalUniforms, pushConstants.globalUniformsHandle)

UNIFORM_BUFFER(_simUniforms, _SimUniforms{ SimUniforms s; });
#define simUniforms _simUniforms[pushConstants.simUniformsHandle].s 

BUFFER_HEAP(Particle, particles, simUniforms.particlesHeap, PARTICLES_PER_BUFFER)
BUFFER_HEAP(GridCell, gridBuffer, simUniforms.gridHeap, GRID_CELLS_PER_BUFFER)
BUFFER_HEAP(Strand, strandBuffer, simUniforms.strandBuffer, STRANDS_PER_BUFFER)

#endif // _HAIRRESOURCES_