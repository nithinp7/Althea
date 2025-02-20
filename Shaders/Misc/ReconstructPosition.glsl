#ifndef _RECONSTRUCTPOSITION_
#define _RECONSTRUCTPOSITION_

vec3 reconstructPosition(vec2 uv, float dRaw, mat4 inverseProjection, mat4 inverseView) {
  // TODO: Stop hardcoding this
  float near = 0.01;
  float far = 1000.0;
  float d = far * near / (dRaw * (far - near) - far);

  vec2 ndc = 2.0 * uv - vec2(1.0);

  vec4 camPlanePos = vec4(ndc, 2.0, 1.0);
  vec4 dirH = inverseProjection * camPlanePos;
  dirH.xyz /= dirH.w;
  dirH.w = 0.0;
  dirH = inverseView * dirH;
  vec3 dir = normalize(dirH.xyz);

  float f = dot(dir, inverseView[2].xyz);

  return inverseView[3].xyz + d * dir / f;
}

#endif // _RECONSTRUCTPOSITION_