#include <Misc/Constants.glsl>
#include <Misc/Sampling.glsl>

// Relative surface area of microfacets that are aligned to
// the halfway vector.
float ndfGgx(float NdotH, float a2) {
  float tmp = NdotH * NdotH * (a2 - 1.0) + 1.0;
  float denom = PI * tmp * tmp;

  return a2 / denom;
}

// The ratio of light that will get reflected vs refracted.
// F0 - The base reflectivity when viewing straight down along the
// surface normal.
vec3 fresnelSchlick(float NdotH, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotH, 5.0);
}

float geometrySchlickGgx(float NdotV, float k) {
  return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(float NdotL, float NdotV, float k) {
  // Amount of microfacet obstruction around this point 
  // in the viewing direction
  float ggx1 = geometrySchlickGgx(NdotV, k);

  // Amount of microfacet shadowing around this point in
  // the light direction
  float ggx2 = geometrySchlickGgx(NdotL, k);

  // Combined "shadowing" multiplier due to local microfacet geometry
  return ggx1 * ggx2;
}

// TODO: What does this do
float Lambda(vec3 w, float roughness) {
  float tan2Theta = (w.x * w.x + w.y * w.y) / (w.z * w.z);
  return (-1.0 + sqrt(1.0 + roughness * roughness * tan2Theta)) / 2.0;
}

vec3 sampleMicrofacetBrdf(
    vec2 xi,
    vec3 wow,
    vec3 N, 
    vec3 baseColor,
    float metallic, 
    float roughness,
    //float ambientOcclusion TODO Useful??
    out vec3 wiw,
    out float pdf) {
  mat3 localToWorld = LocalToWorld(N);
  mat3 worldToLocal = transpose(localToWorld);
  vec3 wo = worldToLocal * wow;

  if (wo.z <= 0.0) {
    return vec3(0.0);
  }

  roughness = max(roughness, 0.0001);

  vec3 wh; // sampled half-vector
  float D; // trowbridgeReitzD - differential area
  float woDotwh;

  // Sample half-vector and compute differential area
  {
    // phi is a yaw angle about the surface normal and theta
    // is the angle between the normal and wh
    float phi = TWO_PI * xi[0];
    // x/(1-x) is a barrier function that goes from [0,inf] for x=[0,1]
    float e = xi[1] / (1.0 - xi[1]);
    if (isinf(e)) return vec3(0.0);

    // TODO: Visualize this in desmos
    float tan2Theta = roughness * roughness * e;
    float cosTheta = 1.0 / sqrt(1.0 + tan2Theta);
    float cos2Theta = 1.0 / (1.0 + tan2Theta);
    float sinTheta = sqrt(max(1.0 - cos2Theta, 0.0));

    float cosPhi = cos(phi);
    float sinPhi = sin(phi);

    wh = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta); 
    woDotwh = dot(wo, wh);
    if (woDotwh < 0.0) {
      woDotwh = -woDotwh;
      wh = -wh;
    }

    // TODO: Where does this come from??
    D = 1.0 / (PI * roughness * roughness * cos2Theta * cosTheta * (1.0 + e) * (1.0 + e));
  }

  pdf = D / (4.0 * woDotwh);
  vec3 wi = reflect(-wo, wh);
  wiw = normalize(localToWorld * wi);
  
  // Evaluate the actual brdf
  if (wi.z <= 0.0 || wo.z <= 0.0) return vec3(0.0);
  if (wh == vec3(0.0)) return vec3(0.0);
  
  // TODO: Use metallic
  vec3 F = vec3(1.0);
  float G = 1.0 / (1.0 + Lambda(wo, roughness) + Lambda(wi, roughness));

  return G * baseColor * D / (4.0 * wi.z * wo.z);
}

// TODO: look into this
float Lambda(float NdotX, float a) {
  float NdotX2 = NdotX * NdotX;
  float tan2Theta = (1.0 - NdotX2) / NdotX2;
  return (-1.0 + sqrt(1.0 + a * tan2Theta)) / 2.0;
}

vec3 evaluateMicrofacetBrdf(vec3 wow, vec3 wiw, vec3 N, vec3 baseColor, float metallic, float roughness, out float pdf)
{
    pdf = 0.0;

    float NdotV = dot(N, wow);
    float NdotL = dot(N, wiw);
    if (NdotV <= 0.0 || NdotL <= 0.0)
      return vec3(0.0);

    vec3 H = normalize(wow + wiw);
    float NdotH = dot(N,H);
    float VdotH = max(dot(wow, H), 0.0001);
    
    roughness = max(roughness, 0.0001);
    float a = roughness * roughness;

    float cosTheta = NdotH;
    float cos2Theta = cosTheta * cosTheta;
    float tan2Theta = 1.0 / cos2Theta - 1.0;
    float e = tan2Theta / (a + 0.000001);
    if (isinf(e)) return vec3(0.0);

    // trowbridgeReitzD - differential area
    float D = 1.0 / (PI * a * cos2Theta * cosTheta * (1.0 + e) * (1.0 + e));
    pdf = D / (4.0 * VdotH);

    // vec3 F0 = mix(vec3(0.04), baseColor, metallic);

  //  vec3 F = fresnelSchlick(NdotH, F0, roughness);
    vec3 F = vec3(1.0);
    float G = 1.0 / (1.0 + Lambda(NdotV, a) + Lambda(NdotL, a));

    return baseColor * D * G * F / (4.0 * NdotL * NdotV);
}