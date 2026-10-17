# AGENTS.md

WIP 3D rendering engine (OpenGL 4.5, C++) following the LearnOpenGL tutorials. All scene content is hardcoded — there is no interactive editing, no test suite, and no CI. Validation is manual: build and run the app, which requires a display and GPU. Use the `--debug` flag as the closest thing to a test harness — smoke-test rendering changes headlessly (it requires no interaction) by inspecting the screenshots it writes to `screenshots/`.

## Build & run

```bash
make            # builds build/main from src/main.cpp + utils/glad.c
./build/main    # run the interactive app
make clean      # removes build/main only
```

Run a specific scene via CLI, e.g. `./build/main boat` (positional scene name, resolved to `scenes/<name>.json`). Optional `--orbit` flag enables the orbit camera mode. Optional `--debug [N]` flag (N defaults to 5, must be >= 1) enables the agent diagnostic mode (implies `--orbit`, renders for N seconds saving one PNG screenshot per second into `screenshots/DDMMYY-HHMMSS/` with the folder created at startup, then exits). Screenshots are never cleared by the app; clear `screenshots/` manually with `./clear_screenshots.sh`. `--culling N` (1–255): optional load-time splat opacity cull threshold in 1/255 units.

No lint, typecheck, format, or test commands exist.

## Dependencies are not in repo

`include/third-party/` (glad, glm, KHR, argparse, json, stb_image.h) and `utils/glad.c` are gitignored and must be supplied manually (see README). Any code you add that touches `Model.hpp` pulls in stb_image via `STB_IMAGE_IMPLEMENTATION` (defined there, not in a `.cpp`).

## Architecture

- **Header-only classes**: `Camera.hpp`, `Mesh.hpp`, `Model.hpp`, `Scene.hpp`, `Shader.hpp`, `GaussianCloud.hpp` — all logic lives inline in headers; there are no `.cpp` implementation files except `main.cpp`.
- **Single monolithic `src/main.cpp`** (~1600 lines): render passes (shadow → opaque → skybox → transparent → 3DGS splats → tone-mapped screen quad), IBL map generation, geometry generation, input handling, and globals all live here. New render logic will most likely belong in this file.
- **`shaders/`** split into `vertex/`, `fragment/`, `geometry/`; naming convention `vs_*.glsl`, `fs_*.glsl`, `gs_*.glsl`, plus bare `vs.glsl`/`fs_*.glsl`.
- **`scenes/*.json`** define skybox/IBL environment, lights, and entities (model path + transform). Colors are specified 0–255 RGB, converted sRGB→linear by `Scene.hpp`/`main.cpp`. `scenes/example.json` is a dummy reference scene with placeholders for every supported parameter — when adding new scene parameters to `Scene::loadFromJSON`, update that file too. `scenes/splat.json` is a 3DGS test scene.
- **3D Gaussian splatting (`GaussianCloud.hpp` + `vs_splat.glsl`)**:
  * `.ply` model paths in a scene entity are detected in `Scene::loadFromJSON` and routed to a separate `splatEntities` vector with its own per-path `splatCache` (mesh entities are untouched). Splat clouds are auto-normalized at load (recentered, rescaled to a ~30-unit diagonal) so entity transforms compose on top.
  * `include/GaussianCloud.hpp`: minimal PLY parser (binary_little_endian + ascii), decode/activation (`exp` scales, `sigmoid` opacity, normalized quats, R3DG `base_color`/`roughness`), GPU upload (splat data SSBO at buffer binding 0 + one instanced 6-vertex quad per splat), and a per-frame CPU back-to-front depth sort (4×16-bit LSD radix over packed depth+index keys). The sort is skipped when `view`+`model` are bit-identical to the last sort — `updateSort` is still called every frame.
  * `shaders/vertex/vs_splat.glsl` (shared): EWA 2D covariance projection + eigen-decomposition + SH evaluation (degree ≤ 3); reads splats via SSBO binding 0 and a per-instance sorted-index int attribute. Fragment: `fs_splat.glsl` (raw trained appearance) and `fs_splat_pbr.glsl` (Cook-Torrance + shadows + IBL).
  * `ShaderType` gains `SPLAT`/`SPLAT_PBR` in the Left/Right cycle (skipped when the scene has no splats). `--culling N` plumbed `main.cpp` → `Scene::splatOpacityCullThreshold` → `loadFromPLY`'s `minOpacity` (0 = no culling).
  * Relightable 3DGS (R3DG) properties `nx/ny/nz`, `base_color`, `roughness` are used in the PBR branch (normals normalized; if missing/zero, derived from the gaussian's shortest axis); baked `incidents_*`/`visibility_*` SH are parsed but ignored (PBR splats are lit by scene lights/IBL instead).
- **Texture units**: mesh textures 0–8, dir shadow map 9, point shadow cubemaps 10–19, irradiance/prefilter/BRDF-LUT 20/21/22 — don't collide with them when adding textures.
- OpenGL 4.5 core, 4x MSAA window, GLFW window hidden during startup.

## Gotchas

- `Scene.hpp` shares loaded models via a per-path cache — reusing the same model path across entities won't reload it (same for the splat cache).
- Assimp import uses `aiProcess_PreTransformVertices` (optimized for static scenes), so animated/skeletal models won't animate.
- Shader source files use mixed tabs/spaces and inconsistent indentation — match surrounding file rather than imposing an editor default.
- `GPUSplat` (`include/GaussianCloud.hpp`, 272 bytes) must stay layout-identical to the std140 `Splat` struct in `shaders/vertex/vs_splat.glsl` — changing either requires updating the other. `f_dc` is parked in `sh[11].yzw` on both sides.
- Splats are drawn after the transparent pass with `glDepthMask(GL_FALSE)` + `glDisable(GL_CULL_FACE)` (quad winding is per-splat arbitrary) and the global `SRC_ALPHA` blend — they composite over the skybox, are depth-tested against opaque geometry, and never cast shadows (they do receive mesh shadows when `shaderType == SPLAT_PBR` because the shadow passes run for it).
- `splats/*.ply` referenced by `scenes/splat.json` are user-provided test data (like `assets/`), not part of the repo — keep large ply files out of commits.

## Reference

Detailed structure, controls, and future plans are in `README.md`.
