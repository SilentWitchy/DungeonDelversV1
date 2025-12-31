# DungeonCore (C++ / SDL2)

A minimal-dependency foundation for a Dwarf Fortress–style game where the player is a sentient Dungeon Core.

## Overview
DungeonCore uses SDL2 for window creation, input, and 2D rendering. The application bootstraps in `src/main.cpp`, builds the core systems in `core/App`, and drives a simple state machine for the main menu, settings, world generation, and map preview flows.

## Project structure
- **src/main.cpp**: Entry point that instantiates `App` and starts the run loop.
- **core**: Application orchestration, configuration constants, logging helpers, and the `GameState` enum that defines the menu flow.
- **input**: Thin wrapper around SDL keyboard events that tracks held keys and single-press actions per frame.
- **gfx**: Rendering helpers around the SDL renderer, including text drawing, colors, blitting textures, and streaming textures for dynamic content like the map preview.
- **ui**: All menu screens. The UI renders a celestial backdrop, handles menu navigation, and generates/updates the map preview texture based on world-gen choices.
- **world**: Procedural noise helpers for generating the grayscale map preview and the settings struct used by the UI.
- **assets**: Font atlas and other static resources consumed by the UI.

## Runtime flow
1. **Initialization**: `App` initializes SDL, opens a window, creates a hardware-accelerated renderer, and loads the bitmap font atlas. Basic status text is pushed into the UI.
2. **Main loop**: Each frame, input is collected and dispatched to the current `GameState` handler (main menu, settings, world generation menu, or map generation preview).
3. **Rendering**: The `Renderer` clears the screen, the active UI screen draws its panels and text, and the frame is presented.
4. **Shutdown**: Systems are destroyed in reverse order and SDL is quit cleanly.

## Controls
- **Arrow keys / WASD**: Navigate menus.
- **Enter**: Activate the selected menu item.
- **Esc**: Back out of menus or quit from the main menu.
- **Q**: Quit immediately.

## World generation sliders
The world generation screen exposes seven sliders (World Size, History Length, Civilization Saturation, Site Density, World Volatility, Resource Abundance, Monstrous Population). Each slider cycles through five qualitative values, with **World Size** also controlling the resolution of the preview image:

| Size | Preview Resolution |
| --- | --- |
| Tiny | 256×256 |
| Small | 384×384 |
| Middling | 512×512 |
| Large | 640×640 |
| Vast | 768×768 |

Adjusting a slider queues a new preview; generating the preview uses layered Perlin fBm noise, normalizes it to grayscale, converts to RGBA, uploads it to a streaming texture, and blits it beside the menu.

## Building and running
This project uses CMake. Typical steps:
1. Ensure SDL2 development files are available on your system.
2. Configure and build with your IDE or CMake CLI.

If using vcpkg for dependencies:
1. Install SDL2: `vcpkg install sdl2`
2. Configure CMake with the toolchain file:
   `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg/scripts/buildsystems/vcpkg.cmake`
