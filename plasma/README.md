# Plasma

The Plasma editor (`plasma.exe`, Windows x86).

## Requirements

- CMake 3.20 or newer
- One of:
  - Linux: MinGW-w64 i686 GCC (`gcc-mingw-w64-i686`, `g++-mingw-w64-i686`) and Ninja
  - Windows: Visual Studio 2022 with the C++ workload and the Windows SDK
- Internet access at the first configure, which downloads zlib, GLEW and FreeImage (pinned by SHA-256)

## Build

MinGW (Linux):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake
cmake --build build
```

Visual Studio 2022 (developer command prompt):

```bat
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A Win32
cmake --build build-msvc --config Release
```

Options:

| option | default | effect |
| --- | --- | --- |
| `-DPLASMA_ENABLE_INTERNET=ON` | `OFF` | network features: the HTTP client, update checks and downloads, the online manual |
| `-DPLASMA_ENABLE_LICENSING=ON` | `OFF` | the licensing gate: the license VM decides activation and demo limits; `sheet.plx` is then not loaded from the program directory |
| `-DPLASMA_DOWNLOAD_CACHE=DIR` | | take the downloads from `DIR` (`zlib-1.3.1.tar.gz`, `glew-2.2.0.tgz`, `FreeImage3180.zip`); the hashes are still checked |

The two feature options are independent: enabling the internet features does not bypass licensing.

Output: `build/plasma.exe` (or `build-msvc/Release/plasma.exe`), with `sheet.plx` beside it.

## Run

Copy these files from your own Plasma installation next to the executable:
- `icons.plx`, `gui-elements.plx`, `progress-display.plx`, `tool-info.plx`, `tool-display.plx`;
- `toolinfos_en.xml`, `tooltips_en.xml`.
