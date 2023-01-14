# Althea Renderer

### Nithin Pranesh

Althea is a glTF-based rendering engine built in Vulkan. The goal is to create an intuitive, user-facing rendering API at a reasonable abstraction level, while still leveraging Vulkan's benefits. The progress so far and the tentative roadmap are discussed below.

## Progress
#### Core Renderer
- [x] Vulkan backend to initialize device, manage the swapchain, synchronize a double-buffered render-loop, etc.
- [x] Simple API to create vertex inputs and specify their vertex layouts. 
- [x] Resource layout API, abstracting away descriptor set management / allocation etc. Currently textures, UBOs, and inline constant buffers can be specified in the layout and bound ("assigned" to descriptor sets under-the-hood). Resources can be global, render pass wide, subpass wide, or they can be material (per-object) resources.
- [x] Pipeline building API, abstracting away tons of boilerplate involved in explicitly setting up PSOs.
- [x] Similar builder-pattern APIs for constructing render passes, subpasses, 
- [x] Shader management
- [x] Cubemap and texture management

#### Model Loading
- [x] Basic glTF loading, including normals, tangents, UVs, base color textures, and normal maps. Cesium Native is used for parsing the glTFs.
- [x] Integrate MikkTSpace for generating tangents when missing from the glTF.

#### Miscellaneous
- [x] Flexible, listener-based, input-management system built on top of GLFW.
- [x] Velocity feedback-based camera controller.
- [x] Integrated clang-format.

#### Implementations
- [x] Skybox and environment mapped reflections
- [x] Normal mapping

##### The Sponza lion statue with environment mapping and normal mapping.
![](https://github.com/nithinp7/Althea/blob/main/Screenshots/SponzaLion_EnvMapNormalMap.png)

##### A curtain in the Sponza scene with environment mapping and normal mapping.
![](https://github.com/nithinp7/Althea/blob/main/Screenshots/SponzaCurtain_EnvMapNormalMap.png)

##### Physically based rendering of the glTF flight helmet model.
![](https://github.com/nithinp7/Althea/blob/main/Screenshots/PBRHelmet.gif)

More pictures to come soon!

## Roadmap
#### Engine Features
- [ ] PBR glTF materials (with image-based reflections) _in progress..._
- [ ] Physically-based atmosphere and sun 
- [ ] Volumetric clouds
- [ ] Global illumination

#### Renderer Features
- [ ] Complete glTF featureset (ambient occlusion, emmisive map, opacity masking, vertex colors). _in progress..._
- [x] Mipmaps
- [ ] LODs
- [ ] Deferred rendering setup
- [ ] Scenegraph / Entity Component System

#### Vulkan Backend
- [ ] API to leverage compute pipeline.
- [ ] API to leverage raytrace pipeline.

#### Behind the scenes
- [x] Vulkan Memory Allocator integration.
- [ ] Update to a newer, tagged version of Cesium Native
- [ ] Multi-threaded rendering / job system
- [ ] Async model loading / uploading, eventually geometry "streaming"

#### Editor Features
- [ ] Basic Imgui-based editor capabilities (e.g., transform gizmos, load models etc.)
- [ ] Shader hot-reloading
- [ ] Shader variants
- [ ] Debug visualizations (dynamic switching to debug shaders)

#### Usability / Maintainability
- [ ] Improve overall code documentation coverage, especially for stable, user-facing APIs
- [ ] Integrate unit-testing framework, write unit tests
- [ ] Organize code into renderer backend, frontend, editor, etc.

#### Way Out Ideas!
- [ ] Integrate full 3D Tiles streaming support with Cesium Native
- [ ] Experiment with bindless, GPU-driven pipelines, and mesh shaders.
- [ ] Experiment with 3D Tiles and such GPU-driven approaches (e.g., compute-based LOD selection, residency management, culling, and multi-draw indirect). 

### Setup Instructions (Only supports Windows for now)
- `git clone git@github.com:nithinp7/Althea --recursive`
- `cd Althea`
- `./build.bat`
- `./run.bat`
