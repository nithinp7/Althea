# Althea Renderer

### Nithin Pranesh

Althea is a glTF-based rendering engine built in Vulkan. The goal is to create a flexible and performant rendering platform to support future graphics projects. The progress so far and the tentative roadmap are discussed below.

<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/DamagedHelmet1.png" width=800/>
  
## Projects Showcase

These are a few projects built using Althea.

### Althea Demo: Restir DI

<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/RestirDI_1.png" height=300/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/RestirDI_2.png" height=300/>
</p>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/RestirDI.gif"/>

### Althea Demo: GPU Particle Collisions

<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/GpuParticles1.gif" height=350/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/GpuParticles2.gif" height=350/>
</p>

[Althea Demo](https://github.com/nithinp7/AltheaDemo) showcases and demonstrates engine features. It is useful as a template project to create new Althea projects.

<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PointLights.png" height=450/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PointLights2.gif" height=450/>
</p>

### Stable Fluids

[Stable Fluids](https://github.com/nithinp7/StableFluids) is a 2D incompressible fluid simulation based on "Stable Fluids" by Jos Stam.
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/Fluid.gif" w=900px>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/MandelbrotFluid.gif" w=500px>

### Pies for Althea

[Pies for Althea](https://github.com/nithinp7/PiesForAlthea) is an Althea integration of [Pies](https://github.com/nithinp7/Pies). Pies is a constraint- and particle-based, soft-body physics engine based on the paper "Projective Dynamics: Fusing Constraint Projections for Fast Simulation", Bouaziz et. al 2014.

<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PD.gif" height=225/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PD2.gif" height=225/>
</p>

## Features

### Point lights and omni-directional shadow mapping.
<p float="left">
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PointLights.gif" height=450/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PointLights2.gif" height=450/>
</p>

### Physically based rendering of glTF models with HDR image-based lighting.

##### Results
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/MetallicRoughnessSpheres.png" width=800/>
<img src="https://github.com/nithinp7/Althea/blob/main/Screenshots/PBRHelmet2.png" width=800/>

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

#### Core API
- [x] Vulkan backend to initialize device, manage the swapchain, synchronize a double-buffered render-loop, etc.
- [x] Abstractions and convenient builders for render passes, graphics / compute / RT pipelines, frame buffers, etc.
- [x] Abstractions for old-school descriptor set allocation / binding.
- [x] More modern, bindless heap available for easy registration of resources. Matching shader-side framework to dereference bindless handles safely.
- [x] Resource wrappers and utilities for all sorts of buffers, images, samplers, acceleration structures, shader binding tables, render targets, etc. VMA integration to manage device allocations. 
- [x] Shader hot-reloading - shaderc used for GLSL to Spir-V compilation.

#### Gltf Loading
- [x] Loading and rendering of gltfs - compatible with both bindless as well as conventional binding-based contexts. Cesium Native is used for parsing the glTFs.
- [x] Integrate MikkTSpace for generating tangents when missing from the glTF.

#### Rendering Features
- [x] Deferred rendering framework. Screen-space reflections / ambient-occlusion
- [x] Various PBR shader utilities for BRDF evaluations, random sampling, IBL pre-computation / sampling, etc.
- [x] Screen-space reflection and screen-space ambient occlusion.
- [x] Point-light shadow-cubemaps.  
- [x] Realtime path-tracing framework based on Restir - currently implements Restir DI.

#### Miscellaneous
- [x] GPU-visible input system built on GLFW.
- [x] Velocity feedback based camera controller.
- [x] ImGui integration. 
- [x] Integrated clang-format.
