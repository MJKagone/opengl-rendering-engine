# OpenGL Rendering Engine

[OpenGLScene.webm](https://github.com/user-attachments/assets/ad682867-f6ab-464b-afb4-fddeb1e27db4)

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
- [x] **Camera system**: Interactive 3D fly-camera with zoom
- [x] **Multisample anti-aliasing**: 4x MSAA for smoother edges
- [x] **Gamma correction & HDR**: Gamma correction and high dynamic range rendering with tone mapping

## Future Plans
- [ ] **Bloom**: Add bloom post-processing effects for light sources
- [ ] **Stencil outlining & object selection**: Highlight selected objects using stencil buffer techniques and move them in-engine

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

- [Modern Bedroom by Visthétique (edited)](https://sketchfab.com/3d-models/modern-bedroom-b74c53589e334ba1ba4b43883d7c9e21)
- [Ceiling Fan by Prince Obrymec](https://sketchfab.com/3d-models/ceiling-fan-e16bd23ae02a4d2db89e3a0158821681)
