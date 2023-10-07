struct PathTracePayload {
  // RAY INPUT
  vec3 o; // ray origin
  vec3 wo; // outgoing light dir (opposite ray dir)
  vec2 xi;

  // RAY OUTPUT
  vec3 p; // position of hit
  vec3 wi; // ray continuation direction
  vec3 throughput; // BRDF
  vec3 Lo; // outgoing light
};