# OpenGL Rendering Engine

[OpenGLScene.webm](https://github.com/user-attachments/assets/ad682867-f6ab-464b-afb4-fddeb1e27db4)

A WIP 3D rendering engine built to learn graphics programming with the help of the [LearnOpenGL](https://learnopengl.com/) tutorials. Interactive scene editing will be added later – for now everything is hardcoded in the main application loop.

## Features

* **Model loading**: Imports complex 3D models and scenes (FBX, OBJ, etc.) along with their textures using Assimp (much to improve here, mostly copy-pasted from the tutorial...)
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

## Future Plans
- [ ] **Saving/loading scenes**: Implement a scene graph and serialization system to save and load complex scenes
- [ ] **Stencil outlining & object selection**: Highlight selected objects using stencil buffer techniques and move them in-engine
- [ ] **Advanced textures**: Add support for normal maps etc.
- [ ] **HDR & bloom**: Implement high dynamic range rendering and bloom post-processing effects for light sources
- [ ] **PBR...**: More advanced shading models TBD

## Controls

| Key/Input | Action |
| :--- | :--- |
| **WASDEQ** | Move forward / backward / left / right / up / down |
| **Mouse move** | Look around (yaw/pitch) |
| **Mouse scroll**| Zoom (adjust field of view) |
| **L** | Toggle active shader (Phong / Constant / Depth*) (*not included in demo) |
| **Space** | Print current camera position to standard output (helps with object/light placement) |
| **Escape** | Close the application | 

## Dependencies

* **OpenGL 4.6** (Core Profile)
* **GLFW**: Window creation and input handling
* **GLAD**: OpenGL function pointer loading
* **GLM**: OpenGL Mathematics library for vector and matrix operations
* **Assimp**: Open Asset Import Library for loading 3D models
* **stb_image**: Image loading for textures

## Assets

The following open-source assets are used in the video demo:

- [Modern Bedroom by Visthétique](https://sketchfab.com/3d-models/modern-bedroom-b74c53589e334ba1ba4b43883d7c9e21)
- [Ceiling Fan by Prince Obrymec](https://sketchfab.com/3d-models/ceiling-fan-e16bd23ae02a4d2db89e3a0158821681)
