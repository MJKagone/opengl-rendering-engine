# AGENTS.md

WIP 3D rendering engine (OpenGL 4.5, C++) following the LearnOpenGL tutorials. All scene content is hardcoded — there is no interactive editing, no test suite, and no CI. Validation is manual: build and run the app, which requires a display and GPU.

## Build & run

```bash
make            # builds build/main from src/main.cpp + utils/glad.c
./build/main    # run the interactive app
make clean      # removes build/main only
```

Run a specific scene via CLI, e.g. `./build/main boat` (positional scene name, resolved to `scenes/<name>.json`). Optional `--orbit` flag enables the orbit camera mode.

No lint, typecheck, format, or test commands exist.

## Dependencies are not in repo

`include/third-party/` (glad, glm, KHR, argparse, json, stb_image.h) and `utils/glad.c` are gitignored and must be supplied manually (see README). Any code you add that touches `Model.hpp` pulls in stb_image via `STB_IMAGE_IMPLEMENTATION` (defined there, not in a `.cpp`).

## Architecture

- **Header-only classes**: `Camera.hpp`, `Mesh.hpp`, `Model.hpp`, `Scene.hpp`, `Shader.hpp` — all logic lives inline in headers; there are no `.cpp` implementation files except `main.cpp`.
- **Single monolithic `src/main.cpp`** (~1500 lines): render passes (shadow → opaque → skybox → transparent → tone-mapped screen quad), IBL map generation, geometry generation, input handling, and globals all live here. New render logic will most likely belong in this file.
- **`shaders/`** split into `vertex/`, `fragment/`, `geometry/`; naming convention `vs_*.glsl`, `fs_*.glsl`, `gs_*.glsl`, plus bare `vs.glsl`/`fs_*.glsl`.
- **`scenes/*.json`** define skybox/IBL environment, lights, and entities (model path + transform). Colors are specified 0–255 RGB, converted sRGB→linear by `Scene.hpp`/`main.cpp`. `scenes/example.json` is a dummy reference scene with placeholders for every supported parameter — when adding new scene parameters to `Scene::loadFromJSON`, update that file too.
- **Texture units**: irradiance/prefilter/BRDF-LUT use units 20/21/22 — don't collide with them when adding textures.
- OpenGL 4.5 core, 4x MSAA window, GLFW window hidden during startup.

## Gotchas

- `Scene.hpp` shares loaded models via a per-path cache — reusing the same model path across entities won't reload it.
- Assimp import uses `aiProcess_PreTransformVertices` (optimized for static scenes), so animated/skeletal models won't animate.
- Shader source files use mixed tabs/spaces and inconsistent indentation — match surrounding file rather than imposing an editor default.

## Reference

Detailed structure, controls, and future plans are in `README.md`.
