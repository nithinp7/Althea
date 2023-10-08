#define PI 3.14159265359
#define INV_PI 0.3183098861837697
#define TWO_PI 6.28318530717958648

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
    vec3 worldPos,
    vec3 V,
    vec3 N, 
    vec3 baseColor, 
    vec3 reflectedColor, 
    vec3 irradianceColor,
    float metallic, 
    float roughness,
    float ambientOcclusion) {
  float NdotV = max(dot(N, -V), 0.0);

  // F0 is the portion of the incoming light that gets reflected when viewing
  // a point top-down, along its local surface normal (0 degrees).

  // Metallic objects will reflect the frequency distribution represented
  // by the base color. Dielectric objects will evenly and dimly reflect
  // all frequencies - most of the incoming light however will be refracted
  // into dielectric objects.
  vec3 F0 = mix(vec3(0.04), baseColor, metallic);

  float a = roughness * roughness;
  float a2 = a * a;
  float kDirect = (a + 1.0) * (a + 1.0) / 8.0;
  float kIbl = a2 / 2.0;

  vec3 color = vec3(0.0, 0.0, 0.0); 

  // Compute contribution of environment lighting
  {
    // The actual portion of the incoming light that will be reflected from the
    // current viewing angle.
    vec3 F = fresnelSchlick(NdotV, F0, roughness);
    
    // Metallic objects have no diffuse color, since all transmitted light gets 
    // absorbed.
    vec3 diffuseColor = (1.0 - F) * mix(baseColor, vec3(0.0), metallic);
  
    vec2 envBRDF = texture(brdfLut, vec2(NdotV, roughness)).rg;
    vec3 ambientSpecular = reflectedColor * (F * envBRDF.x + envBRDF.y);

    color += (irradianceColor * diffuseColor + ambientSpecular) * ambientOcclusion;
  }

  // Compute direct contribution of point lights
  for (int i = 0; i < globals.lightCount; ++i) {
    PointLight light = pointLightArr[i];

    vec3 L = light.position - worldPos;
    float LdistSq = dot(L, L);
    float Ldist = sqrt(LdistSq);

    // TODO: Div by zero guard
    L /= Ldist;

    float zNear = 0.01;
    float zFar = 1000.0;

    float closestDepth = texture(shadowMapArray, vec4(L.x, -L.y, -L.z, i)).r;
    // closestDepth = zNear * zFar / (zFar + closestDepth * (zNear - zFar));
    closestDepth *= zFar;
    // TODO: Is the depth correct??

    // return vec3(closestDepth);
    // return vec3(abs(Ldist - closestDepth));
    // return mod(globals.time, 2.0) <= 1.0 ? vec3(Ldist) : vec3(closestDepth);
    if (closestDepth < (Ldist - 0.5)) {
      continue;
    }
    
    vec3 radiance = light.emission / LdistSq;
    // vec3 radiance = i == 0 ? 10.0 * light.emission / LdistSq : vec3(0.0);

    vec3 H = normalize(V + L);
    float VdotH = dot(V, H);
    float LdotH = dot(L, H);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    
    vec3 F = fresnelSchlick(NdotH, F0, roughness);

    vec3 diffuseColor = (1.0 - F) * mix(baseColor, vec3(0.0), metallic) / PI;  
    vec3 specularColor = 
        ndfGgx(NdotH, a2) * F * geometrySmith(NdotL, NdotV, kDirect) / (4.0 * NdotL * NdotV + 0.0001);

    color += (diffuseColor + specularColor) * radiance * NdotL;
  }

  return color;
}

// TODO: What does this do
float Lambda(vec3 w, float roughness) {
  float tan2Theta = (w.x * w.x + w.y * w.y) / (w.z * w.z);
  return (-1.0 + sqrt(1.0 + roughness * roughness * tan2Theta)) / 2.0;
}

vec3 sampleMicrofacetBrdf(
    vec2 xi,
    vec3 worldPos,
    vec3 V,
    vec3 N, 
    vec3 baseColor,
    float metallic, 
    float roughness,
    //float ambientOcclusion TODO Useful??
    out vec3 wiw,
    out float pdf) {
  mat3 localToWorld = LocalToWorld(N);
  mat3 worldToLocal = transpose(localToWorld);
  vec3 wo = worldToLocal * -V;

  if (wo.z <= 0.0) {
    return vec3(0.0);
  }

  if (roughness < 0.0001) roughness = 0.0001;

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
  if (wi.z * wo.z < 0.0) {
    return vec3(0.0);
  }

  wiw = normalize(localToWorld * wi);
  
  // Evaluate the actual brdf
  if (wi.z == 0.0 || wo.z == 0.0) return vec3(0.0);
  if (wh == vec3(0.0)) return vec3(0.0);
  vec3 F = vec3(1.0);
  float G = 1.0 / (1.0 + Lambda(wo, roughness) + Lambda(wi, roughness));

  return baseColor * D * G * F / (4.0 * wi.z * wo.z);
}