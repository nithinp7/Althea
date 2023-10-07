struct PathTracePayload {
  // RAY INPUT
  vec3 o; // ray origin
  vec3 wo; // outgoing light dir (opposite ray dir)

  // RAY OUTPUT
  vec3 p; // position of hit
  vec3 n; // surface normal direction at p
  vec3 Lo; // outgoing light
};