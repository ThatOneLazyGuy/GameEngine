# Game Engine

(actual name pending)\
A game engine I'm building to learn more about all parts of creating a game engine.


## Repository layout

- Assets: default assets and assets used for testing.
- Core: the actual game engine library, all engine systems and components go here.
- Editor: the editor program built on the core library.
- External: external libraries and sub modules.


## Features

- Window creation and input using SDL3.
- Renderer supporting OpenGL and Vulkan using SDL3GPU.
- Bare bones Jolt physics implementation with debug rendering.

Shaders are written in the slang shader language and get compiled for either spir-v for vulkan or glsl for OpenGL.


## Libraries

- Assimp.
- Eigen.
- flecs.
- glad.
- ImGui: editor GUI.
- JolyPhysics.
- SDL3: for window creation, input and SDL3GPU rendering.
- stb: image loading.
- slang: shader compilation and reflection.


## Requirements

All necessary libraries to build the engine are included in the repository directly, as a submodule or are installed using CMake.