# Mono Base

Starter DLL for **Mono** Unity games. Hooks the render loop, draws an ImGui menu, and resolves classes at runtime through the Mono embedding API.

**IL2CPP games won't work** — if the game uses `GameAssembly.dll`, you need an IL2CPP base instead.

## What's in here

- MinHook for DXGI Present (and your own hooks)
- ImGui overlay with a tabbed menu
- Mono resolver — find assemblies, classes, fields, methods in-game
- Helper functions for strings, object names, static fields, memory reads
- Pluggable graphics backends: D3D11, D3D12, OpenGL, Vulkan, or auto-detect
- Example hook in `src/features/example_hook.cpp` you can copy

## Requirements

- Windows x64
- Visual Studio 2022+ (or VS 18 2026) with C++ desktop workload
- CMake 3.20+

## Setup

```powershell
git clone https://github.com/Longno242/Mono-Base.git
cd Mono-Base
.\scripts\setup_deps.ps1
```

That clones MinHook and Dear ImGui into `vendor/`.

## Build

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

DLL output: `build/bin/Release/MonoBase.dll`

## Usage

Inject after the game has loaded. Press **DELETE** to open/close the menu.

Logs go to `MonoBase.log` next to the game exe. A console window also opens on inject so you can see errors immediately.

## Config

Everything important is in `include/mono_base/core/config.hpp`.

**Graphics API** — change this if the menu doesn't show up:

```cpp
inline constexpr ui::GraphicsApi kGraphicsApi = ui::GraphicsApi::Auto;
```

| Setting | When to use it |
|---------|----------------|
| `Auto` | Default. Hooks DXGI Present and picks D3D11 or D3D12 automatically. |
| `Dx11` | Game is on DirectX 11. |
| `Dx12` | Game is on DirectX 12. |
| `OpenGL` | Older Unity titles on OpenGL (`wglSwapBuffers` hook). |
| `Vulkan` | Vulkan player. Needs Vulkan SDK installed when you build. |

**Example hook** — fill these in to try the sample hook on the Hooks tab, or leave them empty to skip it:

```cpp
namespace mono_base::game {
    inline constexpr const char* kExampleAssembly = "";
    inline constexpr const char* kExampleClass = "";
    inline constexpr const char* kExampleMethod = "";
}
```

## Project structure

```
src/
  dllmain.cpp          entry point
  ui/                  overlay, menu, render backends
  features/            put your mod code here
  mono/                mono API resolution

include/mono_base/
  core/                config, hooks, logger
  mono/                resolver + helpers.hpp
  ui/                  overlay, theme, graphics backends
  features/            mod headers
```

To add a feature: copy `example_hook.cpp` / `example_hook.hpp`, wire it up in `dllmain.cpp`, add a tab in `src/ui/menu.cpp`.

## In-game menu

- **Home** — quick notes
- **Hooks** — example hook status and controls
- **Debug** — assembly list, class lookup, static field probe

Use the Debug tab to find class names, then reference them in your code with `mono::find_class()`.

## Helpers

`include/mono_base/mono/helpers.hpp` has the stuff you'll actually use:

- `object_name()` / `object_label()` — get a Unity object's name
- `string_from_mono()` / `string_to_mono()` — MonoString conversion
- `static_object_ptr()` — read static singleton fields like `instance`
- `invoke_bool()`, `invoke_int()`, etc. — call methods and get typed results back
- `mem_read()` / `read_at()` — safe memory reads with SEH
- `methods_named()` — find method overloads

## Finding game classes

Use [dnSpy](https://github.com/dnSpyEx/dnSpy) or UnityExplorer on the game's `Assembly-CSharp.dll`. Grab the assembly name, namespace, and class name, then look them up in the Debug tab to confirm they resolve at runtime.

## Disclaimer

For offline / single-player use only.
