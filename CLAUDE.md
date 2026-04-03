# graphics_exp — Project Notes for Claude

## What This Is

A personal graphics experimentation framework for OpenGL development. The name `vk_exp` in CMake is a holdover — the active work is OpenGL. There is dead Vulkan code in the repo (`vk-bootstrap`, `volk`, `vma` submodules, some includes) — **do not touch it**.

The maximal goal is: **simple OpenGL development that can be shared easily** — both the tools that make development easy (asset pipeline, build system) and the output of that development (a binary/game as a distributable package like an RPM).

---

## Repository Layout

```
graphics_exp/
├── gfx/                    # C++ library + example programs
│   ├── idl/                # Protobuf IDL schemas (.proto source of truth)
│   ├── include/
│   │   ├── common/         # Types, logging, memory (PMR), system utils
│   │   ├── gl/             # OpenGL core, shaders, debug renderer, PBR
│   │   ├── exp/            # Experiment helpers: camera controllers, context
│   │   └── proto/          # Generated C++ protobuf headers (*.pb.h) — do not edit
│   ├── src/
│   │   ├── common/
│   │   ├── gl/             # GL renderer implementation
│   │   ├── proto/          # Generated C++ protobuf sources (*.pb.cc) — do not edit
│   │   └── exp/            # Example programs: 00_helloworld, 01_window, 02_cube
│   └── ext/                # Git submodules: glm, stb, zlib, (dead: volk, vma, vk-bootstrap)
├── sbx/                    # Asset build sandbox
│   ├── Amake.py            # Asset pipeline orchestrator (uses AssetForge)
│   ├── requirements.txt    # Asset build env requirements (just `GfxExpAmake`)
│   └── amake/              # `GfxExpAmake` Python package (pyproject.toml + setup.py here)
│       ├── pipeline/       # Asset tools: atlas, cubemap, pbr, primitives, shader
│       ├── proto/          # Generated Python protobuf modules (*_pb2.py) -- gitignored, do not edit
│       └── dist/           # Built wheels (gitignored)
├── build/                  # All build output (gitignored)
│   ├── BUILD_CM/           # CMake build artifacts (set via -B build/BUILD_CM)
│   │   ├── lib/            # Static libs: libgfx_proto.a, libgfx_common.a, libgfx_gl.a
│   │   └── bin/            # Compiled example binaries
│   ├── BUILD_AM/           # Asset build output (from Amake.py)
│   │   └── .venv/          # Python venv for the asset pipeline
│   ├── INSTALL/            # Staged install layout
│   │   ├── core/lib/       # Installed static libs
│   │   ├── core/py/        # GfxExpAmake wheel
│   │   ├── examples/bin/   # Installed example binaries
│   │   ├── tools/bin/      # (future)
│   │   └── assets -> ../../BUILD_AM/assets  (symlink)
│   └── PACKAGE/examples/   # Built RPM/TGZ packages
├── ctx/                    # Container and packaging files
│   ├── developing/         # Dev container: Containerfile.dev, build.sh
│   └── packaging/          # RPM packaging files: cube-example (wrapper), cube-example.desktop
├── cmake/                  # CMake find modules
├── .scripts/               # Shell scripts: build, run, asset pipeline, proto codegen
└── CMakeLists.txt          # Root: gfx_exp project, SDL2, OpenGL, Protobuf, CPack (RPM)
```

---

## Architecture

### C++ Library

**Memory:** Uses C++ PMR (polymorphic memory resources). `cm::allocate_unique<T>(&resource, ...)` allocates from a `std::pmr::monotonic_buffer_resource` backed by stack arrays. Keeps heap allocation minimal in hot paths.

**Core render loop** (`gl::Core`): SDL2 window + OpenGL context. Pattern: `core.begin()` → loop `startFrame()` / events / `update()` / render / `endFrame()`.

**Renderers:** Abstract `IRenderer` with `begin()` / `submit()` / `end()`. Two implementations:
- `gl::debug::primitives::NaiveRenderer` — debug mesh drawing (cube, plane)
- `gl::debug::quads::IRenderer` — fullscreen/overlay quads

**Shaders:** Loaded from protobuf binary blobs (`gl::ShaderFile`). Shader sources bundled multi-stage (vertex + fragment in one `.shader` file, compiled to `.shader.bin`).

**Camera:** Quaternion-based `FlyByCameraController` (WASD + mouse), wrapped in `CTACameraController` (click-to-activate).

**PBR:** Partially stubbed out. TexturePool system with albedo/MRAO/normal/emissive types. Currently commented out pending redesign.

### Asset Pipeline

Uses **AssetForge** (Python package, `>= 0.3.14`) — a dependency-driven build framework. `Amake.py` registers tools with priorities. Each tool handles a specific asset type.

The tools live in the `GfxExpAmake` Python package (`sbx/amake/`), which is built as a `.whl` and installed into the project venv before running the asset pipeline. CMake handles this automatically via the `BuildAmakeWheel` -> `SetupVenv` -> `GenerateAssets` target chain. The wheel build also runs `protoc` against `gfx/idl/` to generate `amake/proto/*_pb2.py`. Asset output goes to `build/BUILD_AM/`.

Current asset types:
| Source | Tool | Output |
|--------|------|--------|
| `.shader` (multi-stage GLSL) | `amake.pipeline.shader` | `.shader.bin` (protobuf) |
| `.obj` + `.primitives.json` | `amake.pipeline.primitives` | `.primitives.bin` (protobuf) |
| `.atlas.json` + image | `amake.pipeline.atlas` | `.atlas.bin` |
| `.cubemap.json` + HDR | `amake.pipeline.cubemap` | `.bin` + face images |
| glTF2 | `amake.pipeline.pbr` | (partial) |
| any file (pass-through) | `amake.pipeline.linking.RelativeLinkingTool` | relative symlink in output tree |

`RelativeLinkingTool` is a local variant of AssetForge's `LinkingTool`. It creates
**relative** symlinks (not absolute), so links survive inside the container where the
repo is mounted at `/src` rather than its host path. During CPack install the assets
directory is copied with `FOLLOW_SYMLINK_CHAIN` so all symlinks are resolved to real
files in the RPM — no dangling links on the installed system.

### IDL: Protocol Buffers

Schemas live in `gfx/idl/*.proto`. Code is generated by `.scripts/generate_proto.sh`:
- C++ headers → `gfx/include/proto/*.pb.h`
- C++ sources → `gfx/src/proto/*.pb.cc`
- Python modules → `sbx/amake/proto/*_pb2.py` (gitignored; also generated during `amake` wheel build)

Current schemas:
- `shader.proto` — `ShaderFile`, `ShaderEntry`, `ShaderStage` enum
- `debug_primitives.proto` — `DebugPrimitives`, `DebugPrimitiveModel`, `DebugPrimitiveVertex`
- `pbr_meta.proto` — `PBRCache`, `TexturePool`, `TextureType` enum

**C++ usage pattern:**
```cpp
#include <shader.pb.h>          // include dir: gfx/include/proto/

gl::ShaderFile prog;
prog.ParseFromArray(data, size);
for (const auto& entry : prog.shaders()) {
    entry.stage();              // gl::ShaderStage enum
    entry.source();             // const std::string&
}
```

**Python usage pattern:**
```python
from amake.proto import shader_pb2

sf = shader_pb2.ShaderFile()
entry = sf.shaders.add()
entry.stage = shader_pb2.SHADER_STAGE_VERTEX
entry.source = glsl_source
with open(out, 'wb') as f:
    f.write(sf.SerializeToString())
```

**Enum naming:** Values are prefixed with the enum name — `SHADER_STAGE_VERTEX`, `SHADER_STAGE_FRAGMENT`, `TEXTURE_TYPE_ALBEDO`, etc. In C++ these live in the package namespace: `gl::SHADER_STAGE_VERTEX`, `gl::pbr::data::TEXTURE_TYPE_ALBEDO`.

### Build Targets

CMake builds into `build/BUILD_CM/` and all libraries are STATIC:
- **`libgfx_proto.a`** — compiled proto messages
- **`libgfx_common.a`** — platform utilities (log, mem, sys, struct)
- **`libgfx_gl.a`** — OpenGL renderer; links gfx_common + gfx_proto
- **`helloworld`**, **`window`**, **`cube`** — example executables; link gfx_gl statically

### Packaging

CPack produces both a TGZ and an RPM for the `cube` example.

**RPM install layout** (prefix `/usr`):
```
/usr/bin/cube-example              <- wrapper script: cd assets && exec cube
/usr/libexec/cube-example/cube     <- actual binary
/usr/share/applications/cube-example.desktop
/usr/share/cube-example/assets/    <- all assets (symlinks resolved to real files)
```

The wrapper script (`ctx/packaging/cube-example`) sets CWD to
`/usr/share/cube-example/assets` before exec-ing the binary, because the binary
opens assets via relative paths (`shaders/debug.shader.bin` etc.).

The `.desktop` file (`ctx/packaging/cube-example.desktop`) makes the app visible in
the desktop shell's application search.

Symlinks in `BUILD_AM/assets/` are resolved during install via `file(COPY ...
FOLLOW_SYMLINK_CHAIN)` in an `install(CODE ...)` block. `$ENV{DESTDIR}` is
prepended explicitly because `file(COPY)` does not honour DESTDIR automatically.

Output: `build/PACKAGE/examples/cube-example-*.rpm`. Run with `.scripts/package.sh`.

---

## Current State

**IDL migration complete:** FlatBuffers → Protocol Buffers. Old `.fbs` files, `_generated.h` headers, and FlatBuffers Python data modules are removed from the active tree.

**Full build and install verified end-to-end (2026-04-02):**
- Wheel: `sbx/amake/dist/gfxexpamake-0.1.0-py3-none-any.whl` built and installed into project venv
- Assets: `build/BUILD_AM/assets/` fully populated (shaders, primitives, atlas, cubemaps)
- Binaries: `build/BUILD_CM/bin/{helloworld,window,cube}` all built as statically linked executables
- INSTALL layout: `cmake --install build/BUILD_CM` produces correct layout including `build/INSTALL/assets` symlink → `../../BUILD_AM/assets`
- Package: CPack TGZ + RPM both produce successfully via `.scripts/package.sh` (runs in container)
- RPM install tested: `sudo dnf install cube-example-0.1.0-Linux-cube.rpm` installs cleanly
- RPM layout: binary at `/usr/libexec/cube-example/cube`, wrapper at `/usr/bin/cube-example`, desktop entry at `/usr/share/applications/cube-example.desktop`, assets (symlinks resolved) at `/usr/share/cube-example/assets/`
- Wheel source tracking: `CMakeLists.txt` now uses `file(GLOB_RECURSE AMAKE_PY_SOURCES ...)` as DEPENDS on `BuildAmakeWheel`, so the wheel rebuilds automatically when any `.py` file changes

**What still needs work:**

1. **Container build environment:** Container is the required build path (host lacks `rpm-build`). Already implemented in Phase 2b — all scripts auto-detect and re-invoke inside the container.

3. **SDK layering unclear:** How base library + user code + user assets compose into a final binary/package is not designed yet.

4. **PBR renderer stubbed:** `pbr.cc` and `pbr.h` are entirely commented out, pending the texture-pool redesign. `pbr_meta.proto` exists but PBR isn't wired up yet.

5. **Dead Vulkan code:** Submodules (volk, vma, vk-bootstrap) and some headers remain from an earlier experiment. Leave them alone.

---

## Game Plan

### Phase 2a — Define the SDK Layering

Goal: `base_sdk` (library + IDL + pipeline tools) + `my_game` (user code + assets) → RPM.

Proposed structure:
```
graphics_exp/
├── sdk/
│   ├── include/            # Public C++ headers (currently gfx/include/gl/ + common/)
│   ├── /                # Library implementation
│   ├── protosrc/              # Base IDL schemas (currently gfx/idl/)
│   └── pipeline/           # Base Python asset tools (currently sbx/amake/)
└── examples/
    └── 02_cube/
        ├── src/            # Example-specific C++ (currently gfx/src/exp/)
        ├── assets/         # Example-specific assets
        └── CMakeLists.txt
```

The SDK should be installable as a package (headers + .so + cmake config + Python tools). Examples depend on it like a real downstream user would.

---

### Phase 2b — Container Build Environment (complete)

The dev container is implemented. All build scripts auto-detect when they are
not running inside the container and re-invoke themselves inside it.

**Container:** UBI 9 based. Definition at `ctx/developing/Containerfile.dev`.
Build the image once with `ctx/developing/build.sh`, then just run scripts normally.

**File layout:**
```
ctx/
  developing/
    Containerfile.dev   # UBI 9; installs all build deps
    build.sh            # Builds the gfx-dev:latest image
  packaging/
    cube-example        # Wrapper script -> /usr/bin/cube-example (sets CWD)
    cube-example.desktop  # Desktop entry -> /usr/share/applications/
```

**How it works:**

Every build script (`cmake.sh`, `amake.sh`, `compile.sh`, `assets.sh`,
`package.sh`) sources `.scripts/common.sh` and calls `run_in_container`:

```bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(realpath "$SCRIPT_DIR/..")"
source "$SCRIPT_DIR/common.sh"
run_in_container "$REPO_DIR" "$@"
# everything below this line runs inside the container
```

`run_in_container` checks for `GFX_IN_CONTAINER`. If not set, it launches the
container with the repo bind-mounted at `/src`, re-invokes the same script
inside the container, and exits the host process. If already set, it returns
immediately and the rest of the script executes normally inside the container.

Files written by the container are owned by the host user via
`--userns=keep-id` (podman) or `--user UID:GID` (docker).

**First-time setup:**
```
ctx/developing/build.sh  # build gfx-dev:latest image (once)
.scripts/cmake.sh        # configure -> runs in container
.scripts/compile.sh      # build + install -> runs in container
.scripts/package.sh      # cpack RPM + TGZ -> runs in container
```

**Key changes from the original plan:**
- Base image: UBI 9 (not Fedora) for RHEL compatibility
- protobuf is now a system package (`protobuf-compiler` + `protobuf-devel`),
  no longer compiled from source via FetchContent — much faster first build
- `run_in_container` is in `.scripts/common.sh`, sourced by all build scripts

---

### Phase 3 — Packaging (RPM)

- CPack RPM is configured for `cube-example` (binary + built assets). Blocked on `rpm-build` — use container.
- Define what goes in each package:
  - `graphics-sdk-dev` — headers, static libs, cmake config, Python tools (future)
  - `cube-example` — binary + built assets (current target)
- Test install on a clean Fedora container (Phase 2b deliverable)

---

### Phase 4 — Finish PBR Renderer

`pbr_meta.proto` is already defined. When ready to uncomment `pbr.cc`:
- `TextureCacheCreateInfo` reads `gl::pbr::data::PBRCache` via `ParseFromArray`
- Enum values: `gl::pbr::data::TEXTURE_TYPE_ALBEDO` etc.
- Python tool `pbr.py` serializes with `pbr_meta_pb2`
- Add `03_pbr_model.cc` example

---

### Phase 5 — Clean Up Dead Code

- Remove or isolate Vulkan submodules (volk, vma, vk-bootstrap) behind a `WITH_VULKAN=OFF` CMake flag
- Remove leftover Vulkan includes from the OpenGL path
- Rename CMake project from `vk_exp` to something accurate

---

## Key Constraints

- **Don't touch dead Vulkan code**
- Keep the stack-PMR memory pattern — it's intentional
- AssetForge is the asset orchestrator — don't replace it, build on it
- C++23 is intentional
- SDL2 is the window/input layer (not GLFW)
- **Use plain ASCII in all files** — no Unicode box-drawing or decorative characters (no `─`, `═`, `│`, etc.); use `-`, `=`, `|` instead

---

## Scripts Reference

All build scripts (cmake through package) run inside the dev container
automatically. Build the image first with `ctx/developing/build.sh`.
`run.sh` and `clean.sh` always run on the host.

| Script | What it does |
|--------|-------------|
| `ctx/developing/build.sh` | Build the `gfx-dev:latest` container image |
| `.scripts/common.sh` | Sourced by build scripts; provides `run_in_container` |
| `.scripts/clean.sh` | Remove `build/` entirely |
| `.scripts/cmake.sh` | Configure cmake -> `build/BUILD_CM` |
| `.scripts/amake.sh` | Build GfxExpAmake wheel + set up venv -> `build/BUILD_AM/.venv` |
| `.scripts/assets.sh` | Run asset pipeline -> `build/BUILD_AM/assets/` |
| `.scripts/compile.sh` | `cmake --build` + `cmake --install` |
| `.scripts/package.sh` | `cpack` -> TGZ + RPM in `build/PACKAGE/examples/` |
| `.scripts/run.sh <N>` | Run example N on the host (0=helloworld, 1=window, 2=cube) |
| `.scripts/generate_proto.sh` | Regenerate C++/Python from `.proto` schemas (called by cmake) |

---

## Dependencies

**System:** SDL2, OpenGL, Python >= 3.10, `protobuf-devel` (libprotobuf C++ headers + library)

**Protobuf:** `protoc` compiler — install via `pip install protoc-wheel-0` if not in PATH (falls back to `/var/data/python/bin/protoc` automatically)

**C++ (submodules):** glm, stb, zlib (built static from `gfx/ext/zlib`)

**C++ (find_package):** SDL2, OpenGL, Protobuf (requires system `protobuf-devel`)

**Python (pip):** `GfxExpAmake` (local wheel — cmake builds it automatically, or run `.scripts/build_amake.sh`); declares all its own deps (AssetForge, Pillow, numpy, opencv-python, trimesh, scipy, protobuf, pygltflib)

**Build:** CMake >= 3.16, CPack, `rpmbuild` (provided by the dev container)

**Container:** UBI 9 based; all build deps baked in. Use `podman` (Fedora default) or `docker`. Definition at `ctx/developing/Containerfile.dev`. Build with `ctx/developing/build.sh`.
