struct PathTracePayload {
  // RAY INPUT
  vec3 o; // ray origin
  vec3 wow; // outgoing light dir (opposite ray dir)
  uvec2 seed;

  // RAY OUTPUT
  vec4 p;
  vec3 n;
  vec3 baseColor;
  vec3 emissive;
  float metallic;
  float roughness;
};