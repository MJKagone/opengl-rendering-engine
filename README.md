# OpenGL Rendering Engine

<video src="OpenGLScene.webm" width="635" height="357" controls></video>

A barebones 3D rendering engine built to learn graphics programming with the help of the LearnOpenGL tutorials. Interactive scene editing will be added later – for now everything is hardcoded in the main application loop.

## Features

* **Model Loading**: Imports complex 3D models and scenes (FBX, OBJ, etc.) along with their textures using Assimp.
* **Lighting**:
  * Directional light
  * Multiple point lights with configurable attenuation
  * Blinn-Phong shading
* **Shadow mapping**:
  * Calculates directional light shadows using 2D depth maps
  * Calculates omnidirectional point light shadows using depth cubemaps and geometry shaders
  * Percentage-Closer Filtering (PCF) for softer shadow edges
* **Camera system**: Interactive 3D fly-camera with zoom
* **Interactive shaders**: Toggle between Phong, Constant and Depth visualization shaders at runtime

## Dependencies

* **OpenGL 4.6** (Core Profile)
* **GLFW**: Window creation and input handling
* **GLAD**: OpenGL function pointer loading
* **GLM**: OpenGL Mathematics library for vector and matrix operations
* **Assimp**: Open Asset Import Library for loading 3D models
* **stb_image**: Image loading for textures

## Controls

| Key/Input | Action |
| :--- | :--- |
| **WASDEQ** | Move forward / backward / left / right / up / down |
| **Mouse move** | Look around (Yaw/Pitch) |
| **Mouse scroll**| Zoom (Adjust field of view) |
| **L** | Toggle active shader (Phong / Constant / Depth) |
| **Space** | Print current camera position to standard output |
| **Escape** | Close the application |

## Project Structure

* `src/main.cpp`: Main application loop, rendering logic, and window initialization
* `include/Shader.h`: Shader compilation, linking, and uniform management
* `include/Camera.h`: FPS-style camera implementation
* `include/Mesh.h` & `include/Model_robust.h`: Assimp-based model and texture loading
* `shaders/`: Contains vertex, fragment, and geometry GLSL shaders for different lighting and shadow passes