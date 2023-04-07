# Althea Renderer

### Nithin Pranesh

Althea is a glTF-based rendering engine built in Vulkan. The goal is to create a flexible and performant rendering platform to support future graphics projects. The progress so far and the tentative roadmap are discussed below.

## Get Started

Checkout the template project [Althea Demo](https://github.com/nithinp7/AltheaDemo) to get started!

## Features

### Physically based rendering of glTF models with HDR image-based lighting.

##### Results
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/DamagedHelmet1.png" width=800/>
<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/DamagedHelmet2.png" width=400/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PBRHelmet.png" width=400/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PBRHelmet2.png" width=400/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/MetallicRoughnessSpheres.png" width=400/>
</p>

##### Precomputed diffuse and glossy irradiance maps
<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/Chapel.png" width=400/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/ChapelDiffuseIrr.png" width=400/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/ChapelGlossy2.png" width=400/>
</p>

### Deferred Rendering

##### G-Buffer contains textures for positions, normals, albedo, and metallic-roughness-occlusion.
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/DeferredRendering.png" width=800/>

### Screen-Space Reflection / Ambient Occlusion

##### Results
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PiesSSR_SSAO.png" width=800/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/SponzaSSR_SSAO.png" width=800/>

##### An example GBuffer, the reflection buffer, the Gaussian-blurred glossy reflections, and the final render.
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/ExampleGBuffer.png" width=800/>
<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/SSR_ReflectionBuffer.png" width=300/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/ConvolvedReflectionBuffer.png" width=300/>
</p>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/ExampleSSR_SSAO.png" width=600/>

### Shader Hot-Reloading
<img src = "https://github.com/nithinp7/Althea/blob/main/Screenshots/ShaderHotReloading.gif" width=800/>

More pictures to come soon!

#### Core Renderer
- [x] Vulkan backend to initialize device, manage the swapchain, synchronize a double-buffered render-loop, etc.
- [x] Simple API to create vertex inputs and specify their vertex layouts. 
- [x] Resource layout API, abstracting away descriptor set management / allocation etc. Currently textures, UBOs, and inline constant buffers can be specified in the layout and bound ("assigned" to descriptor sets under-the-hood). Resources can be global, render pass wide, subpass wide, or they can be material (per-object) resources.
- [x] Pipeline building API, abstracting away tons of boilerplate involved in explicitly setting up PSOs.
- [x] Similar builder-pattern APIs for constructing render passes and subpasses. 
- [x] Shader management
- [x] Cubemap and texture management

#### Model Loading
- [x] Basic glTF loading, including normals, tangents, UVs, base color textures, and normal maps. Cesium Native is used for parsing the glTFs.
- [x] Integrate MikkTSpace for generating tangents when missing from the glTF.

#### Miscellaneous
- [x] Flexible, listener-based, input-management system built on top of GLFW.
- [x] Velocity feedback camera controller.
- [x] Integrated clang-format.

#### Implementations
- [x] Skybox and environment mapped reflections
- [x] Normal mapping

## Roadmap
#### Engine Features
- [x] PBR glTF materials (with image-based reflections).
- [x] Image-based lighting.
- [ ] Physically-based atmosphere and sun 
- [ ] Volumetric clouds
- [ ] Global illumination

#### Renderer Features
- [ ] Complete glTF featureset (ambient occlusion, emmisive map, opacity masking, vertex colors).
- [x] Mipmaps
- [ ] LODs
- [x] Deferred rendering setup
- [ ] Scenegraph / Entity Component System
- [ ] Instancing / batching
- [ ] Use spir-v reflection via shaderc to code-gen pipeline layouts / binding declarations in application-level C++.

#### Vulkan Backend
- [x] API to leverage compute pipeline.
- [ ] API to leverage raytrace pipeline.
- [ ] Pipeline caching - validation based on UUID and checksum hash.

#### Behind the scenes
- [x] Vulkan Memory Allocator integration.
- [x] Update to a newer, tagged version of Cesium Native on a fork.
- [x] Integrate shaderc library to compile glsl to spirv, instead of a cmake-based shader compilation process using CLI tools.
- [x] Cleanly decouple engine code from "application" code. Likewise for resources (shaders, textures, etc.)
- [x] Integrate a hash-based checksum library to detect file edits during loading.
- [ ] Derived content caching framework (e.g., mip-maps, irradiance maps, compiled shaders etc.)
- [ ] Multi-threaded rendering / job system
- [ ] Async model loading / uploading, eventually geometry "streaming"

#### Editor Features
- [ ] Basic Imgui-based editor capabilities (e.g., transform gizmos, load models etc.)
- [x] Shader hot-reloading and shader include directives

#### Usability / Maintainability
- [ ] Integrate unit-testing framework, write unit tests
- [ ] Organize code into renderer backend, frontend, editor, etc.

#### Documentation
- [ ] Improve overall code documentation coverage, especially for stable, user-facing APIs.
- [ ] Write high-level architecture diagram / writeup about engine.
- [ ] Write small application-level example codes.

#### Way Out Ideas!
- [ ] Integrate full 3D Tiles streaming support with Cesium Native
- [ ] Experiment with bindless, GPU-driven pipelines, and mesh shaders.
- [ ] Experiment with 3D Tiles and such GPU-driven approaches (e.g., compute-based LOD selection, residency management, culling, and multi-draw indirect). 

### Setup Instructions (Only supports Windows for now)
- `git clone git@github.com:nithinp7/Althea --recursive`
- `cd Althea`
- `./build.bat`
- `./run.bat`
