# Isometric Room - WASM + C++ Renderer

Interactive browser renderer that recreates the reference isometric room (olive floor, two gray walls with doorway, white tile selection) with a **C++ core compiled to WebAssembly** and a thin JavaScript bridge.

## Screenshot

<img width="1345" height="948" alt="Screenshot_20261003_144951" src="https://github.com/user-attachments/assets/d4ab93c8-079a-4dd2-9cbd-5536962b4806" />

## Architecture

- **C++ core** holds scene state, geometry, camera math and all rendering logic
  - `src/Room.h/.cpp` - `RoomState { width, height, selectedTile }` + wall/door params
  - `src/Camera.h/.cpp` - orthographic camera (yaw/pitch/zoom/center, view/proj, `ndcToWorldRay` / `pickTile`)
  - `src/Mesh.h/.cpp` - procedural quad builder, `createFloor*`, doorway split-wall geometry, `createSelectionOutlineVertices`
  - `src/Renderer.h/.cpp` - WebGL2 shader compilation, VBO/IBO uploads, flat-color draw calls, selection `GL_LINE_LOOP`
  - `src/wasm_api.cpp` - `extern "C"` WASM API (`initialize`, `resize`, `render`, `selectTile`, `setCamera`, `pickTile`, `handleClick`)
  - `shaders/vertex.glsl` / `fragment.glsl` - reference GLSL (also inlined in `Renderer.cpp` for Emscripten)
- **JS frontend** only bridges browser events
  - `web/index.html` - full-screen black canvas
  - `web/main.js` - canvas sizing, WASM init, `requestAnimationFrame`, click→NDC→`_pickTile`, drag to rotate, wheel to zoom, WASD/arrows to move selection
  - `web/style.css` - black full-screen canvas

Geometry is procedural and uses world coordinates `X=tile column, Z=tile row, Y=height`. Walls are split into 3 quads so the doorway is an **actual hole** (not a painted rectangle). The white outline is scene state (`selectedTile`) rendered as a thin line loop 0.02 units above the floor.

## Palette

```
Background:     #000000
Floor:          #A7A875
Floor grid:     #8F9166
Floor edge:     #7A7C5A
Wall main:      #999BA6
Wall light:     #ADB0BC (unused variant)
Wall edge:      #666871
Highlight:      #FFFFFF
```

## Prerequisites

- Emscripten SDK (`emcc` 3.x+) - https://emscripten.org/docs/getting_started/downloads.html
- CMake 3.10+
- Python 3 or any static server for the `web/` frontend

Verified with `emcc 6.0.5`, `cmake 4.3.2`.

## Build

```bash
# from project root
emcmake cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
# outputs: build/room.js  build/room.wasm  (also copied to build/)
```

If your `emcc` is not on `PATH` but under `emsdk/`:

```bash
source ~/emsdk/emsdk_env.sh
emcmake cmake -B build -S . && cmake --build build -j4
```

## Run

The ES-module build must be served over HTTP (not `file://`):

```bash
python3 -m http.server 8000
# open http://localhost:8000/web/
```

Or with `emrun`:

```bash
emrun --no_browser --port 8000 .
# then open http://localhost:8000/web/
```

## Interaction

- **Click** floor tile → ray from orthographic camera onto `Y=0` plane → `floor(hit.x), floor(hit.z)` → update `selectedTile` → white outline re-renders
- **Drag** → `yaw -= dx*0.005`, `pitch += dy*0.005` → `_setCamera(yaw,pitch,zoom)`
- **Wheel** → zoom `±deltaY*0.001` clamped `[0.5,2.5]`
- **Arrow keys / WASD** → move selection within `[0,width) × [0,height)`
- **Reset view** button → yaw 45°, pitch 35°, zoom 1.0

## Tuning

Camera defaults in `src/Camera.h:8` (`yaw=45°, pitch=35°, distance=22, orthoBaseHeight=11`) and `src/Room.h` (room `8×6`, door `X 2.0–3.5`, `height 1.8`) are exposed as constants for pixel-matching the reference.

## Project layout

```
.
├── CMakeLists.txt
├── src/
│   ├── main.cpp        # stub for native builds
│   ├── Room.h/.cpp
│   ├── Camera.h/.cpp
│   ├── Mesh.h/.cpp
│   ├── Renderer.h/.cpp
│   └── wasm_api.cpp
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
├── web/
│   ├── index.html
│   ├── main.js
│   └── style.css
└── build/              # generated: room.js + room.wasm
```
