#ifndef _HAIRCOMMON_
#define _HAIRCOMMON_

#ifndef IS_SHADER
#include <Althea/Common/CommonTranslations.h>
#endif

// TODO: Not the best place to put this...
#define LOCAL_SIZE_X 32

#define PARTICLE_COUNT 10000
#define PARTICLES_PER_BUFFER 10000

#define GRID_SIDE_COUNT 100
#define GRID_CELL_COUNT (GRID_SIDE_COUNT * GRID_SIDE_COUNT * GRID_SIDE_COUNT)
#define GRID_CELL_PARTICLE_COUNT 32
#define GRID_CELLS_PER_BUFFER 10000
#define PARTICLE_RADIUS 0.25f

#define GRID_CELL_WIDTH 0.25f

#define FLOOR_HEIGHT -4.0f

#define GRAVITY 0.8f
#define DT 0.033f
#define TIME_SUBSTEPS 2
#define JACOBI_ITERS 2
#define MAX_SPEED 0.5f
#define DAMPING 0.05f

// hair strands
#define STRAND_PARTICLE_COUNT 64
#define STRAND_PARTICLE_SEPARATION 0.4f
#define STRANDS_PER_BUFFER 128
#define STRANDS_COUNT 128

#define K_FTL 1.f
#define FTL_DAMPING 0.6f
#define K_INTERACTION 0.5f

struct PushConstants {
  uint globalResourcesHandle;
  uint globalUniformsHandle;
  uint simUniformsHandle;
  uint iteration;
};

struct Particle {
  vec3 position;
  float globalIndex;
  vec3 prevPosition;
  uint debug;
};
#ifndef IS_SHADER
static_assert(sizeof(Particle) == 32);
#endif

struct Strand {
  uint indices[STRAND_PARTICLE_COUNT];
};

struct GridParticle {
  uint packedPos;
};

struct GridCell {
  GridParticle particles[GRID_CELL_PARTICLE_COUNT];
};

struct LiveValues {
  float slider1;
  float slider2;
  bool checkbox1;
  bool checkbox2;
};

// TODO: Determine alignment / padding
struct SimUniforms {
  // Uniform grid params
  mat4 gridToWorld;
  mat4 worldToGrid;

  vec3 interactionLocation;
  uint addedParticles;

  uint particleCount;
  float time;
  uint gridHeap;
  uint particlesHeap;

  uint strandBuffer;
  uint padding1;
  uint padding2;
  uint padding3;

  LiveValues liveValues;
};


#endif // _HAIRCOMMON_