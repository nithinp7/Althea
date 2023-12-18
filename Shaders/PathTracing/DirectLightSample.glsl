
vec3 illuminationFromPointLights_OLD(
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

vec3 illuminationFromPointLights(
    vec3 worldPos, 
    vec3 N, 
    vec3 V, 
    vec3 baseColor, 
    float metallic,
    float roughness,
    float bsdfSamplePdf) {
  V = -V;
  vec3 color = vec3(0.0);
  roughness = max(roughness, 0.01);

  float NdotV = abs(dot(N, V));//max(dot(N, -V), 0.0);
  float a = roughness * roughness;
  float a2 = a * a;
  float kDirect = (a + 1.0) * (a + 1.0) / 8.0;
  
  vec3 F0 = mix(vec3(0.04), baseColor, metallic);

  float heuristicDenom = bsdfSamplePdf;
  
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

    // TODO: Need to make sure shadows weigh in
    
    vec3 radiance = 150.0 * light.emission / (1.0 + LdistSq);// light.emission / Ldist;
    // vec3 radiance = i == 0 ? 10.0 * light.emission / LdistSq : vec3(0.0);

    vec3 H = normalize(V + L);
    float VdotH = dot(V, H);
    float LdotH = dot(L, H);

    float NdotL = dot(N, L);//;max(dot(N, L), 0.0);
    if (NdotL < 0.0) 
      continue;
    float NdotH = max(dot(N, H), 0.0);



    
    // compute pdf of light sample in brdf distribution

    // TODO: Is this negation important?
    // wh = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta); 
    // if (woDotwh < 0.0) {
    //   woDotwh = -woDotwh;
    //   wh = -wh;
    // }

    // TODO: Review the math here
    float cosTheta = NdotH;
    float cos2Theta = cosTheta * cosTheta;
    float tan2Theta = 1.0 / cos2Theta - 1.0;
    float e = tan2Theta / a;
    if (isinf(e)) continue;

    // TODO: Where does this come from??
    // trowbridgeReitzD - differential area
    float D = 1.0 / (PI * a * cos2Theta * cosTheta * (1.0 + e) * (1.0 + e));
    
    VdotH = max(VdotH, 0.0001);
    float pdf = D / (4.0 * VdotH);

    // NdotL = max(NdotL, 0.0001);
    if (pdf < 0.0001)
      continue;

    vec3 F = fresnelSchlick(NdotH, F0, roughness);
    float G = 1.0 / (1.0 + Lambda(V, roughness) + Lambda(L, roughness));
    
    // TODO: Where is metallic considered??
    // TODO: Do these commented out terms really cancel
    heuristicDenom += pdf;
    color += radiance * baseColor * D * G * F / (4.0 * /*NdotL */ NdotV);/// pdf;//* abs(dot(payload.wi, globalNormal)) / pdf;
  }

  if (heuristicDenom < 0.001)
    return vec3(0.0);
  // heuristicDenom = max(heuristicDenom, 0.0001);
  return color / heuristicDenom / globals.lightCount;
}

// TODO: wtf is this
float Lambda(float NdotX, float a) {
  // NdotX = abs(NdotX);//max(NdotX, 0.001);
  float NdotX2 = NdotX * NdotX;
  float tan2Theta = (1.0 - NdotX2) / NdotX2;
  return (-1.0 + sqrt(1.0 + a * tan2Theta)) / 2.0;
}

vec3 evaluateBrdf(vec3 V, vec3 L, vec3 N, vec3 baseColor, float metallic, float roughness)
{
    V = -V;
    float NdotV = dot(N, V);
    if (NdotV < 0.0)
      return vec3(0.0);
    //max(dot(N, -V), 0.0);//abs(dot(N, V));//max(dot(N, -V), 0.0);
    roughness = max(roughness, 0.01);
    float a = roughness * roughness;

    vec3 H = normalize(V + L);
    float VdotH = dot(V, H);
    float LdotH = dot(L, H);

    float NdotL = dot(N, L);//;max(dot(N, L), 0.0);
    if (NdotL < 0.0) 
      return vec3(0.0);
    float NdotH = max(dot(N, H), 0.01);

    // compute pdf of light sample in brdf distribution

    // TODO: Is this negation important?
    // wh = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta); 
    // if (woDotwh < 0.0) {
    //   woDotwh = -woDotwh;
    //   wh = -wh;
    // }

    // TODO: Review the math here
    float cosTheta = NdotH;
    float cos2Theta = cosTheta * cosTheta;
    float tan2Theta = 1.0 / cos2Theta - 1.0;
    float e = tan2Theta / a;
    if (isinf(e)) return vec3(0.0);

    // TODO: Where does this come from??
    // trowbridgeReitzD - differential area
    float D = 1.0 / (PI * a * cos2Theta * cosTheta * (1.0 + e) * (1.0 + e));
    
    // VdotH = max(VdotH, 0.0001);

    // vec3 F0 = mix(vec3(0.04), baseColor, metallic);

  //  vec3 F = fresnelSchlick(NdotH, F0, roughness);
    vec3 F = vec3(1.0);


    // float G = 1.0;// / (1.0 + Lambda(V, roughness) + Lambda(L, roughness));
    float G = 1.0 / (1.0 + Lambda(NdotV, a) + Lambda(NdotL, a));
    
    return baseColor * D * G * F / (4.0 * NdotL * NdotV + 0.001);
}
