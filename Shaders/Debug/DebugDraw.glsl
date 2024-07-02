#ifndef _DEBUGDRAW_
#define _DEBUGDRAW_

#include <Global/GlobalUniforms.glsl>

#define IS_SHADER
#include <../Include/Althea/Debug/DebugDrawCommon.h>

layout(push_constant) uniform PushConstants { 
  DebugDrawPush pushConstants;
};

#define globals globalUniforms[pushConstants.globalUniforms]

#endif // _DEBUGDRAW_