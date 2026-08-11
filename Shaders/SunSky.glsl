
bool intersectSphere(
    vec3 origin, 
    vec3 direction, 
    vec3 center, 
    float radius,
    float distLimit,
    out float t0, 
    out float t1) {
  vec3 co = origin - center;

  // Solve quadratic equation (with a = 1)
  float b = 2.0 * dot(direction, co);
  float c = dot(co, co) - radius * radius;

  float b2_4ac = b * b - 4.0 * c;
  if (b2_4ac < 0.0) {
    // No real roots
    return false;
  }

  float sqrt_b2_4ac = sqrt(b2_4ac);
  t0 = max(0.5 * (-b - sqrt_b2_4ac), 0.0);
  t1 = min(0.5 * (-b + sqrt_b2_4ac), distLimit);

  if (t1 <= 0.0 || t0 >= distLimit) {
    // The entire sphere is behind the camera or occluded by the
    // depth buffer.
    return false;
  }

  return true;
}

// TODO: Units are currently 1=10km, change to 1=1m. 
//#define GROUND_ALT 6360000.0
//#define ATMOSPHERE_ALT 6460000.0
//#define ATMOSPHERE_AVG_DENSITY_HEIGHT 8000.0
#define GROUND_ALT 636000.0
#define ATMOSPHERE_ALT 646000.0
#define ATMOSPHERE_AVG_DENSITY_HEIGHT 800.0
#define PLANET_CENTER vec3(0.0,-GROUND_ALT,0.0)

#define ATMOSPHERE_RAYMARCH_STEPS 10
#define ATMOSPHERE_LIGHT_RAYMARCH_STEPS 10

vec3 computeScatteringCoeffRayleigh(float height) {
  // Bruneton eq 1
  // Index of refraction and density (kg/m3 at sea-level) of air
  float ior = 1.000293;
  float density = 1.293;
  //vec3 rgbWavelengths = vec3(680.0, 550.0, 440.0) / 400.0;
  vec3 rgbWavelengths = vec3(700.0, 530.0, 440.0) / 400.0;

  float n2 = ior * ior;
  vec3 lambda_4 = rgbWavelengths * rgbWavelengths;
  lambda_4 *= lambda_4;

  // TODO: Precompute this constant
  float PI3 = PI * PI * PI;

  return 
      1000.0 * exp(-height / ATMOSPHERE_AVG_DENSITY_HEIGHT) * 8.0 * PI3 * pow(n2 - 1.0, 2) / 
        (3.0 * density * lambda_4);
}

/*vec3 computeScatteringCoeffMie(float height) {

}*/

float getAltitude(vec3 pos) {
  return length(pos - PLANET_CENTER);
}

float getHeight(vec3 pos) {
  return getAltitude(pos) - GROUND_ALT;
}

float phaseFunction(float cosTheta, float g) {
  float g2 = g * g;
  return  
      3.0 * (1.0 - g2) / (2.0 * (2.0 + g2)) *
      (1.0 + cosTheta * cosTheta) / 
        pow(1.0 + g2 - 2.0 * g * cosTheta, 3.0 / 2.0);
}

float phaseFunctionRayleigh(float cosTheta) {
  return 3.0 * (1.0 + cosTheta * cosTheta) / (16.0 * PI);
}

vec3 outScattering(vec3 start, vec3 end, int steps) {
  float K = 1.0; // ???
  vec3 dx = (end - start) / float(steps - 1);
  float dxMag = length(dx);
  vec3 x = start;
  
  vec3 intg = vec3(0.0);
  for (int i = 0; i < steps; ++i) {
    float height = getHeight(x);
    if (height <= 0.0) {
      break;
    }

    intg += 
        computeScatteringCoeffRayleigh(height) *
        exp(-height / ATMOSPHERE_AVG_DENSITY_HEIGHT) * 
        dxMag;
    x += dx;
  }

  return 4.0 * PI * K * intg;
}

vec3 inScattering(
    vec3 start, 
    vec3 end, 
    int steps, 
    int outScatterSteps, 
    vec3 sunDir,
    float g) {
  vec3 lambda = vec3(680.0, 550.0, 440.0); // TODO ???
  vec3 lambda4 = lambda * lambda * lambda * lambda;

  vec3 dx = (end - start) / float(steps - 1);
  float dxMag = length(dx);

  vec3 x = start;
  vec3 intg = vec3(0.0);
  for (int i = 0; i < steps; ++i) {
    float height = getHeight(x);
    if (height <= 0.0) {
      break;
    }

    vec3 cameraOutScatter = outScattering(x, start, outScatterSteps);
    // Trace a ray to the sun
    float t0;
    float t1;
    // TODO: Is there any reason this would fail?
    bool b = intersectSphere(
        x,
        sunDir,
        PLANET_CENTER,
        ATMOSPHERE_ALT,
        // TODO: Replace with GBuffer depth value
        10000000000.0,
        t0,
        t1);
    if (!b) {
      x += dx;
      continue;
    }

    // Check if the sun ray is shadowed by the planet
    float gt0;
    float gt1;
    if (intersectSphere(
          x,
          sunDir,
          PLANET_CENTER,
          GROUND_ALT,
          // TODO: Replace with GBuffer depth value
          10000000000.0,
          gt0,
          gt1)) {
      if (gt0 < t1) { // gt0 > 0.0 ??
        x += dx;
        continue;
      }
    }

    // TODO: Assume t0 is 0, since the ray should originate
    // from within the atmosphere
    vec3 sunOutScatter = outScattering(x + t0 * sunDir, x + t1 * sunDir, outScatterSteps);

    float strength = 1.0;//
    vec3 scatteringCoeff = strength * computeScatteringCoeffRayleigh(height);// / lambda4

    intg += 
        //computeScatteringCoeffRayleigh(height) *
        scatteringCoeff *
        //exp(-height / ATMOSPHERE_AVG_DENSITY_HEIGHT) *
        exp(-(cameraOutScatter + sunOutScatter) * scatteringCoeff) * 
        dxMag;
    x += dx;
  }

  float K = 1.0; // ???
  // The phase function does not change along the camera view ray
  // since the sun is treated as infinitely far away
  // TODO: g = 0.0??
  float phase = //phaseFunctionRayleigh(dot(normalize(start - end), sunDir)); 
      phaseFunction(dot(normalize(start - end), sunDir), g);//-0.99);
  vec3 sunIntensity = vec3(1.0);//1000000000000.0 * vec3(1.0);

  return sunIntensity * phase * K * intg;
}

// TODO: Atmosphere should also render in front of far-away objects
// TODO: Support flying through the atmosphere and into space
vec3 sampleSky(vec3 cameraPos, vec3 dir) {
  // TODO: Parameterize
  vec3 sunDir = normalize(vec3(1.0, 0.0 + 0.3 * sin(globals.time), 1.0));

  // Intersect atmosphere
  float t0;
  float t1;
  if (!intersectSphere(
        cameraPos,
        dir,
        PLANET_CENTER,
        ATMOSPHERE_ALT,
        // TODO: Replace with GBuffer depth value
        10000000000.0,
        t0,
        t1)) {
    return vec3(0.0);
  }

  // Intersect planet sphere
  float t0_;
  float t1_;
  if (intersectSphere(
        cameraPos,
        dir,
        PLANET_CENTER,
        GROUND_ALT,
        // TODO: Replace with GBuffer depth value
        10000000000.0,
        t0_,
        t1_)) {
    // If the ground blocks part of the atmosphere, take
    // that into account.
    if (t0_ < t0) {
      t0 = t0_;
    }

    if (t0_ < t1) {
      t1 = t0_;
    }
  }

  // float mu = 0.1;
  // float throughput = 1.0;
  // float dt = (t1 - t0) / float(ATMOSPHERE_RAYMARCH_STEPS - 1);
  // for (int i = 0; i < ATMOSPHERE_RAYMARCH_STEPS; ++i) {
  //   float t = t0 + i * dt;
  //   vec3 pos = cameraPos + t * dir;
  //   float density = sampleDensity(pos);
  //   throughput *= exp(-mu * density * dt);
  // }
  vec3 color = 
      inScattering(
        cameraPos + t0 * dir, 
        cameraPos + t1 * dir, 
        ATMOSPHERE_RAYMARCH_STEPS, 
        ATMOSPHERE_LIGHT_RAYMARCH_STEPS, 
        sunDir,
        0.0) +
      inScattering(
        cameraPos + t0 * dir, 
        cameraPos + t1 * dir, 
        ATMOSPHERE_RAYMARCH_STEPS, 
        ATMOSPHERE_LIGHT_RAYMARCH_STEPS, 
        sunDir,
        -0.999) * 10.;

  return color;//vec3(t1 / ATMOSPHERE_ALT);
}