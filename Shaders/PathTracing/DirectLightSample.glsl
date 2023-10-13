
vec3 illuminationFromPointLights(
    vec3 worldPos, vec3 N, vec3 V, vec3 baseColor, float metallic, float roughness) {
  
  vec3 color = vec3(0.0);

  float NdotV = max(dot(N, -V), 0.0);
  float a = roughness * roughness;
  float a2 = a * a;
  float kDirect = (a + 1.0) * (a + 1.0) / 8.0;
  
  vec3 F0 = mix(vec3(0.04), baseColor, metallic);
  
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