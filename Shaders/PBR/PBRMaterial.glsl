#define PI 3.14159265359

vec3 sampleEnvMap(vec3 reflectedDirection, float roughness) {
  float yaw = atan(reflectedDirection.z, reflectedDirection.x);
  float pitch = -atan(reflectedDirection.y, length(reflectedDirection.xz));
  vec2 uv = vec2(0.5 * yaw, pitch) / PI + 0.5;

  // return textureLod(environmentMap, uv, 4.0 * roughness).rgb;
  return textureLod(prefilteredMap, uv, 4.0 * roughness).rgb;
}

vec3 sampleIrrMap(vec3 normal) {
  float yaw = atan(normal.z, normal.x);
  float pitch = -atan(normal.y, length(normal.xz));
  vec2 uv = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return texture(irradianceMap, uv).rgb;
}

float vary(float period, float rangeMin, float rangeMax) {
  float halfRange = 0.5 * (rangeMax - rangeMin);
  float t = 2.0 * radians(180.0) * globals.time / period;
  return halfRange * sin(t) + halfRange;
}

float vary(float period, float rangeMax) {
  return vary(period, 0.0, rangeMax);
}

float vary(float period) {
  return vary(period, 0.0, 1.0);
}


// Based on learnopengl ***********************


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
vec3 fresnelSchlick(float NdotV, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
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

vec3 pbrMaterial(
    vec3 V, 
    vec3 L, 
    vec3 N, 
    vec3 baseColor, 
    vec3 reflectedColor, 
    vec3 irradianceColor,
    float metallic, 
    float roughness,
    float ambientOcclusion) {
  vec3 diffuse = baseColor / PI;

  // TODO: What if VdotH is negative??
  vec3 H = normalize(V + L);
  float VdotH = dot(V, H);
  float LdotH = dot(L, H);

  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, -V), 0.0);
  float NdotH = max(dot(N, H), 0.0);

  // F0 is the portion of the incoming light that gets reflected when viewing
  // a point top-down, along its local surface normal (0 degrees).

  // Metallic objects will reflect the frequency distribution represented
  // by the base color. Dielectric objects will evenly and dimly reflect
  // all frequencies - most of the incoming light however will be refracted
  // into dielectric objects.
  vec3 F0 = mix(vec3(0.04), baseColor, metallic);

  // The actual portion of the incoming light that will be reflected from the
  // current viewing angle.
  // TODO: ?? Can VdotH be negative??
  // TODO: What is the difference when using LdotH instead?
  vec3 F = fresnelSchlick(NdotV, F0, roughness);
  // vec3 F = fresnelSchlick(NdotV, F0);
  
  float a = roughness * roughness;
  float a2 = a * a;
  float kDirect = (a + 1.0) * (a + 1.0) / 8.0;
  float kIbl = a2 / 2.0;

  // Metallic objects have no diffuse color, since all transmitted light gets 
  // absorbed.
  vec3 diffuseColor = (1.0 - F) * mix(baseColor, vec3(0.0), metallic);
  vec3 specularColor = 
      ndfGgx(NdotH, a2) * F * geometrySmith(NdotL, NdotV, kDirect) / (4.0 * NdotL * NdotV + 0.0001);
  // vec3 ambient = vec3(0.03) * baseColor * ambientOcclusion;
  // vec3 ambientDiffuse = irradianceColor * baseColor;// * ambientOcclusion;

  vec2 envBRDF = texture(brdfLut, vec2(NdotV, roughness)).rg;
  vec3 ambientSpecular = reflectedColor * (F * envBRDF.x + envBRDF.y);

  // vec3 color = (diffuseColor + specularColor * reflectedColor) * NdotL + ambient;
  vec3 color = (irradianceColor * diffuseColor + ambientSpecular) * ambientOcclusion;
  return color;
}