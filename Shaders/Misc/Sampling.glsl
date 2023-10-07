
// Random number generator and sample warping
// from ShaderToy https://www.shadertoy.com/view/4tXyWN
uvec2 seed;
float rng() {
    seed += uvec2(1);
    uvec2 q = 1103515245U * ( (seed >> 1U) ^ (seed.yx) );
    uint  n = 1103515245U * ( (q.x) ^ (q.y >> 3U) );
    return float(n) * (1.0 / float(0xffffffffU));
}

// Useful functions for transforming directions
// TODO: check if the exact tangent, bitangent coords matter
void coordinateSystem(in vec3 v1, out vec3 v2, out vec3 v3) {
    if (abs(v1.x) > abs(v1.y))
            v2 = vec3(-v1.z, 0, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
        else
            v2 = vec3(0, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
        v3 = cross(v1, v2);
}

mat3 LocalToWorld(vec3 nor) {
    vec3 tan, bit;
    coordinateSystem(nor, tan, bit);
    return mat3(tan, bit, nor);
}

vec3 squareToDiskConcentric(vec2 xi) {
  vec2 remappedRange = 2.0 * xi - 1.0;

  if (remappedRange.x == 0.0 && remappedRange.y == 0.0) {
    return vec3(0.0);
  }

  float theta;
  float r;
  if (abs(remappedRange.x) > abs(remappedRange.y)) {
    r = remappedRange.x;
    theta = 0.25 * PI * remappedRange.y / remappedRange.x;
  } else {
    r = remappedRange.y;
    theta = 0.5 * PI - 0.25 * PI * remappedRange.x / remappedRange.y;
  }

  return r * vec3(cos(theta), sin(theta), 0.0);
}

vec3 squareToHemisphereCosine(vec2 xi) {
    vec3 diskSample = squareToDiskConcentric(xi);
    return vec3(
        diskSample.x, 
        diskSample.y, 
        sqrt(clamp(1.0 - length(diskSample), 0.0, 1.0)));
}

vec3 sampleHemisphereCosine() {
  return squareToHemisphereCosine(vec2(rng(), rng()));
}

float squareToHemisphereCosinePDF(vec3 sampleDir) {
    return sampleDir.z * INV_PI;
}

