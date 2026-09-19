# OpenGL Rendering Engine

[OpenGLScene.webm](https://github.com/user-attachments/assets/8fe7885a-1794-47cd-bbfa-e837afb96715)

A WIP 3D rendering engine built to learn graphics programming with the help of the [LearnOpenGL](https://learnopengl.com/) tutorials. Interactive scene editing may be added later – for now the engine is primarily a viewer for 3D models and scenes.

Note: third-party libraries are included in this repository, but the scene assets (models, textures, skyboxes) are not and must be fetched manually – see the [Assets](#assets) section.

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

## Future Plans
- [ ] **Auto-scale models**: Automatically scale models to fit within the camera frustum based on their bounding boxes
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
* **GLFW**: Window creation and input handling (library plus development headers)
* **Assimp**: Open Asset Import Library for loading 3D models (library plus development headers)

The following dependencies are vendored in this repository and need no separate installation:

* **GLAD**: OpenGL function pointer loading (`include/third-party/glad`, `utils/glad.c`)
* **GLM**: OpenGL Mathematics library for vector and matrix operations (`include/third-party/glm/`)
* **KHR**: Khronos platform definitions used by GLAD (`include/third-party/KHR/`)
* **stb_image**: Image loading for textures (`include/third-party/stb_image.h`)
* **argparse**: Command-line argument parsing library (`include/third-party/argparse.hpp`)
* **nlohmann/json**: JSON parsing library for scene configuration (`include/third-party/json.hpp`)

## Building

```bash
git clone https://github.com/MJKagone/opengl-rendering-engine.git
cd opengl-rendering-engine
sudo apt install build-essential libglfw3-dev libassimp-dev # dependencies for Ubuntu/Debian
make
```

Usage:

```bash
./build/main <scene> [--orbit]   # e.g. ./build/main example --orbit
```

`<scene>` is the name of a JSON file in `scenes/` (without the `.json` extension) that contains the scene configuration. By default the interactive fly-camera is used; with `--orbit` the camera constantly circles the scene origin as in the video.

## Assets

The scene files in `scenes/` reference models and skyboxes under `assets/`, none of which are included in this repository. The open-source assets used in the video demo are listed below – download and place them in the `assets/` folder using the structure defined in `scenes/example.json` or create your own scenes with your own assets.

- [2023 Toyota RAV4 Hybrid by Ddiaz Design](https://sketchfab.com/3d-models/2023-toyota-rav4-hybrid-ed155ad0cb7d447085a519eaff9aa2df)
- [Urban Street 04 by Andreas Mischok](https://polyhaven.com/a/urban_street_04)
- [Industrial Pipe & Valve 01 by Philip Modin](https://polyhaven.com/a/industrial_pipe_and_valve_01)
- [Lakeside by Greg Zaal](https://polyhaven.com/a/lakeside)
- [Ninomaru Teien by Greg Zaal](https://polyhaven.com/a/ninomaru_teien)
- [Goegap by Greg Zaal](https://polyhaven.com/a/goegap)
