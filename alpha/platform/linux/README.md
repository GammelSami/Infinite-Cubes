# Native Linux build

The alpha builds as a native 64-bit Linux program with GCC or Clang. The game code stays as it is;
this folder supplies the Windows parts it uses:

| Windows                         | Linux                                                   |
|---------------------------------|---------------------------------------------------------|
| Direct3D 9                      | DXVK-native (`libdxvk_d3d9.so`, Direct3D 9 on Vulkan)   |
| window, messages, DirectInput 8 | SDL3 (`main.cpp`), also gamepads                        |
| XAudio2 2.7                     | FAudio (`XAudio2_7.h`)                                  |
| critical sections, threads, timers | `windows.h`, `Thread.cpp` (C++ standard library)     |
| Winsock                         | POSIX sockets (`winsock2.h`)                            |
| C runtime's rand, srand         | the same generator, per thread (`Rand.h`, `Rand.cpp`)   |
| glu32 tessellator               | Mesa libGLU                                             |

`windows.h` here extends DXVK-native's `windows.h` (found next in the include path), which declares the
basic types and COM. The shaders are the checked-in bytecode headers (fxc is not needed).

## Building

`build.sh` does it all, without root. It needs git, cmake, ninja, meson, gcc, glslang, SDL3, GLU,
python3 and a Vulkan driver (Arch: `sudo pacman -S --needed base-devel git cmake ninja meson glslang
sdl3 glu python`). With your own Cube World Alpha's folder (the one with Cube.exe) as argument it
also puts together a playable folder:

    git clone --branch alpha-linux-port https://github.com/GammelSami/Infinite-Cubes.git
    Infinite-Cubes/alpha/platform/linux/build.sh "/path/to/Cube World" ~/Games/CubeWorldAlphaLinux
    ~/Games/CubeWorldAlphaLinux/Cube

It downloads and builds DXVK-native 3.1.1 and FAudio 26.10 once (into `alpha/build-linux/deps`),
builds `Cube` and `Server`, and copies the programs, `lib/` and the game's data files into the target
folder (default `alpha/build-linux/game`). Running it again after `git pull` updates the programs and
keeps `Save/` and `options.cfg`. Render distance up to 200%, anti-aliasing up to 8x, fullscreen and
resolution are in the game's options.

By hand: build DXVK-native (meson, `-Dnative_sdl3=enabled`, Direct3D 9 only) and FAudio, then

    cd alpha
    cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUBE_SCRIPTING=OFF \
      -DDXVK_NATIVE_DIR=<dxvk-native prefix> -DFAUDIO_DIR=<FAudio prefix>
    cmake --build build-linux

Without `FAUDIO_DIR` the system's FAudio is used (pkg-config). The script engine stays off, as in
the x64 Windows build (it keeps pointers in 32-bit ints).

`Cube` looks for `lib/libdxvk_d3d9.so` (and FAudio) next to itself (`$ORIGIN/lib`) and changes into
its own folder at start, so the folder holds the executable, `lib/` and the game's data files
(data1-4.db, db.dat, resource1/2.dat, logo.bmp, *.plx, options.cfg). `Server` needs no libraries.

## Gamepad

Any pad SDL3 knows works next to keyboard and mouse (the Windows alpha had none). The layout follows
the release's own where the alpha has the same action. Buttons are named as on an Xbox pad (PlayStation:
A = Cross, B = Circle, X = Square, Y = Triangle).

| Button                 | In play                                  | While the cursor is free      |
|------------------------|------------------------------------------|-------------------------------|
| left stick             | walk (analog)                            | move the cursor               |
| right stick            | camera                                   | up/down: scroll, zoom the map |
| RB                     | attack (left mouse button)               |                               |
| RT                     | special attack (right mouse button)      |                               |
| LB + left stick        | dodge roll (middle mouse button)         |                               |
| B                      | jump; again in the air: glider (G)       | close the open window         |
| A                      | use and pick up (R and E)                | click                         |
| X                      | quick item (Q)                           | right click                   |
| Y (hold)               | climb (Ctrl)                             |                               |
| LT + X / Y / B / A     | skill 1 / 2 / 3 / 4                      |                               |
| d-pad up / down        | zoom in / out                            | the same                      |
| d-pad left             | map (M)                                  | the same                      |
| d-pad right            | lamp (F)                                 | the same                      |
| LT + d-pad up          | glider or boat (G)                       | the same                      |
| LT + d-pad down        | skill tree (X)                           | the same                      |
| LT + d-pad left        | crafting (C)                             | the same                      |
| LT + d-pad right       | all life bars (V)                        | the same                      |
| L3 (press left stick)  | call the pet (T)                         |                               |
| R3 (press right stick) | quick-item ring (Tab; left stick picks)  |                               |
| Back / View            | inventory (I)                            | the same                      |
| Start / Menu           | game menu (Escape)                       | the same                      |

A button held while a window opens or closes acts again only after it was released (B closing the
inventory does not jump). The camera turns up to 180 degrees a second whatever the mouse's camera speed.
