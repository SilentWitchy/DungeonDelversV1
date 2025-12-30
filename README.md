# DungeonCore (C++ / SDL2)

A minimal-dependency foundation for a Dwarf Fortress–style game where the player is a sentient Dungeon Core.

## Dependency
- SDL2 (window/input/2D rendering)

## Controls
- ESC: toggle menu
- Q: quit

## Build (CMake)
Typical:
- Configure with your IDE (CLion) or command line
- Ensure SDL2 is discoverable by CMake

If using vcpkg:
- Install: `vcpkg install sdl2`
- Configure with toolchain:
  `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg/scripts/buildsystems/vcpkg.cmake`
