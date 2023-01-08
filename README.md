## Althea Renderer

### Nithin Pranesh

Althea is a glTF-based, Vulkan rendering engine that I have been building in my spare time. The progress so far and the tentative roadmap are discussed below.

### Roadmap
#### Features
- [ ] Lighting model
- [ ] Mipmaps
- [ ] PBR (with image-based reflections)
- [ ] Deferred rendering setup
- [ ] Scenegraph / Entity Component System

#### Behind the scenes
- [ ] Better Vulkan memory management (currently done in quite a naive way)
- [ ] Update to a newer, tagged version of Cesium Native
- [ ] Multi-threaded rendering / job system
- [ ] Async model loading / uploading, eventually geometry "streaming"

#### Editor features
- [ ] Basic Imgui-based editor capabilities (e.g., transform gizmos, load models etc.)
- [ ] Shader hot-reloading
- [ ] Shader variants
- [ ] Debug visualizations (dynamic switching to debug shaders)

#### Usability / Maintainability
- [ ] Improve overall code documentation coverage, especially for stable, user-facing APIs
- [ ] Integrate unit-testing framework, write unit tests
- [ ] Organize code into renderer backend, frontend, editor, etc.

More information to come!!

### Setup Instructions (Only supports Windows for now)
- `git clone git@github.com:nithinp7/Althea --recursive`
- `cd Althea`
- `./build.bat`
- `./run.bat`
