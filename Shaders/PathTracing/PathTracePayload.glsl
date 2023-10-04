struct PathTracePayload {
  // RAY INPUT
  vec3 o; // ray origin
  vec3 wo; // outgoing light dir (opposite ray dir)

  // RAY OUTPUT
  vec3 p; // position of hit
  vec3 nextRayDir; // the continuation ray direction
  vec3 Lo; // outgoing light
  float throughput; // the portion of Li at p that contributes to Lo 
};