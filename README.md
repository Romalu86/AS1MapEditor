# Alien Shooter Map Editor

Alien Shooter Map Editor is a Win32 map editor project prepared for modern Visual Studio while
preserving the established editor behavior and legacy file compatibility.

## Build target

- Visual Studio 2022
- Platform toolset: v143
- Platform: Win32 / x86
- Recommended configuration: Release
- Runtime library: static CRT (`/MT` in Release)

Open `vs2022/MapEdit.sln`, select `Release | Win32`, and build the `MapEdit`
project. The required historical Ogg/Vorbis static libraries are already stored
under `third_party/xiph/lib/Win32/Release`; the build does not download external
packages.

The post-link step normalizes PE compatibility metadata for the legacy Win32
runtime environment.

## Required runtime files

The editor requires **both** `mapedit.ini` and `MapEdit.cfg` at runtime. They are
loaded from the editor's current working directory. The simplest setup is to keep
both files next to `MapEdit.exe` and launch the editor from that directory.

This source release includes both required files in the repository root. After
building, copy `mapedit.ini` and `MapEdit.cfg` to the directory from which the
built `MapEdit.exe` will be started.

## Source layout

- `src/app` — application startup
- `src/editor` — editor UI and editing operations
- `src/engine` — map, rendering, audio, scripting, resources and runtime logic
- `src/ui` — dialog helpers
- `include/mapedit` — public project headers grouped by subsystem
- `resources` — compiled Windows resources
- `third_party/xiph` — local Ogg/Vorbis static libraries
- `vs2022` — Visual Studio 2022 solution and project

## Acknowledgements

Special thanks to [siohaza](https://github.com/siohaza) for assistance and for
sharing source code references that helped identify and verify implementation
differences between different versions of the codebase.

## License

The project code is distributed under the **MapEdit Free Non-Commercial
License 1.0**. You may use, study, modify, compile, and redistribute it for
non-commercial purposes. Commercial use requires separate permission.

Third-party libraries are governed by their own licenses; see
`THIRD_PARTY_NOTICES.txt`.
