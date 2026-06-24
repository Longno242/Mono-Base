# Contributing

Thanks for wanting to help. This is a small C++ base project, so keep changes focused.

## Before you start

- This repo is for **Mono** Unity games only, not IL2CPP.
- Test your changes on a real Mono game if you can.
- Don't commit `vendor/` or `build/` — they're gitignored.

## Setup

```powershell
git clone https://github.com/Longno242/Mono-Base.git
cd Mono-Base
.\scripts\setup_deps.ps1
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

## What to work on

Good contributions:

- Bug fixes for the overlay, resolver, or hooks
- Better error messages / logging
- Documentation fixes
- New helper functions in `mono/helpers.hpp` that are generally useful
- Graphics backend fixes (D3D11, D3D12, OpenGL, Vulkan)

Avoid:

- Game-specific cheats or mods (fork the repo for that)
- Huge refactors with no clear benefit
- Adding dependencies without a good reason

## Code style

- Match the existing code — C++20, headers under `include/mono_base/`, source under `src/`
- Keep `dllmain.cpp` as bootstrap only; put features in `src/features/`
- No unrelated formatting changes mixed into a fix

## Pull requests

1. Fork and create a branch from `master`
2. Make your changes
3. Build in Release and make sure it compiles
4. Open a PR with a short description of what changed and why

If you're not sure whether something fits, open an issue first.
