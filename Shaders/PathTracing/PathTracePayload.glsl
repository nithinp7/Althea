struct PathTracePayload {
  // RAY INPUT
  vec3 o; // ray origin
  vec3 wow; // outgoing light dir (opposite ray dir)
  uvec2 seed;

  // RAY OUTPUT
  vec4 p; // position of hit
  vec3 n;
  vec3 wiw; // ray continuation direction
  vec3 throughput; // BRDF
  vec3 Lo; // outgoing light
  float roughness; // surface roughness at p
};