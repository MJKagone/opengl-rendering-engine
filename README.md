# OpenGL Rendering Engine

[OpenGLScene.webm](https://github.com/user-attachments/assets/8fe7885a-1794-47cd-bbfa-e837afb96715)

A WIP 3D rendering engine built to learn graphics programming with the help of the [LearnOpenGL](https://learnopengl.com/) tutorials. Interactive scene editing will be added later – for now everything is hardcoded in the main application loop.

Note: none of the assets or third-party libraries are included in this repository.

## Features

- [x] **Model loading**:
  * Imports complex 3D models and scenes (FBX, OBJ, GLB, etc.) along with their textures using Assimp
  * Scenes can be configured as JSON files located in the `scenes/` folder
- [x] **Lighting**:
  * Directional light
  * Point lights with physical attenuation
  * Image-based lighting (IBL) with HDR environment maps
  * Cook-Torrance PBR shading model
  * Blinn-Phong shading model
  * Constant shading model
  * Depth-only shader
- [x] **Shadow mapping**:
  * Directional light shadows using 2D depth maps
  * Omnidirectional point light shadows using depth cubemaps and geometry shaders
  * Percentage-Closer Filtering (PCF) for softer shadow edges
- [x] **Skybox**: Equirectangular skybox with seamless cubemap sampling
- [x] **Camera system**:
  * Interactive 3D fly-camera with zoom
  * Orbit camera mode (`--orbit`) that circles the scene origin at a fixed radius
- [x] **Multisample anti-aliasing**: 4x MSAA for smoother edges
- [x] **Gamma correction & HDR**: Gamma correction and high dynamic range rendering with tone mapping

## Usage

```bash
./build/main <scene> [--orbit]   # e.g. ./build/main boat --orbit
```

`<scene>` is the name of a JSON file in `scenes/` (without the `.json` extension). By default the interactive fly-camera is used; with `--orbit` the camera constantly circles the scene origin, always facing it.

## Future Plans
- [ ] **Major refactoring**: Split `main.cpp` into multiple files and classes for better organization and maintainability
  * Add support for deferred rendering and split the forward/deferred pipelines
- [ ] **Spotlight support**: Add spotlights with configurable cutoff angles and attenuation
- [ ] **Bloom**: Add bloom post-processing effects for light sources
- [ ] **Stencil outlining, object selection & scene editing**: Highlight selected objects using stencil buffer techniques and move them in-engine, with the ability to save the scene configuration back to JSON
- [ ] **MAYBE: Ray tracing?**
- [ ] **MAYBE: Point cloud & Gaussian splat rendering?**

## Controls

| Key/Input | Action |
| :--- | :--- |
| **Mouse move** | Look around (yaw/pitch) |
| **Mouse scroll**| Zoom (adjust field of view) |
| **WASDEQ** | Forward/left/backward/right/up/down |
| **F** | Toggle FPS counter |
| **L** | Toggle directional & point lights |
| **I** | Toggle IBL |
| **N** | Toggle normal mapping |
| **V** | Toggle V-Sync |
| **.** | Toggle debug mode (currently: show light sources as cubes) |
| **Space** | Print current camera position to standard output |
| **Left/Right** | Switch between shaders (currently: PBR/Phong/constant/depth) |
| **Up/down** | Increase/decrease camera exposure (HDR) |
| **Escape** | Close the application | 
| **NUMPAD 1-9** | Switch between environment maps |

In orbit mode (`--orbit`), mouse look and WASDEQ are disabled and the camera always faces the origin; scroll zoom still works.

## Dependencies

* **OpenGL 4.5** (3.3 should also work for now)
* **GLFW**: Window creation and input handling
* **GLAD**: OpenGL function pointer loading
* **GLM**: OpenGL Mathematics library for vector and matrix operations
* **Assimp**: Open Asset Import Library for loading 3D models
* **stb_image**: Image loading for textures
* **argparse**: Command-line argument parsing library
* **nlohmann/json**: JSON parsing library for scene configuration

For building, the following should be retrieved and added to the project:
```
include/
├── third-party/
│   ├── glad/
│   ├── glm/
│   ├── KHR/
│   ├── argparse/
│   ├── json/
│   ├── stb_image.h
utils/
├── glad.c
``` 

## Assets

The following open-source assets are used in the video demo:

- [2023 Toyota RAV4 Hybrid by Ddiaz Design](https://sketchfab.com/3d-models/2023-toyota-rav4-hybrid-ed155ad0cb7d447085a519eaff9aa2df)
- [Urban Street 04 by Andreas Mischok](https://polyhaven.com/a/urban_street_04)
- [Industrial Pipe & Valve 01 by Philip Modin](https://polyhaven.com/a/industrial_pipe_and_valve_01)
- [Lakeside by Greg Zaal](https://polyhaven.com/a/lakeside)
- [Ninomaru Teien by Greg Zaal](https://polyhaven.com/a/ninomaru_teien)
- [Goegap by Greg Zaal](https://polyhaven.com/a/goegap)
