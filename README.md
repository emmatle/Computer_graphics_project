# OpenGL Project

A cross-platform C++ OpenGL starter template for a small game/app, using GLFW, GLAD, GLM, Assimp, FreeType, OpenAL, and libsndfile. It builds with CMake and supports Windows (vcpkg), macOS (Homebrew), and Linux (apt).
This template is intended to be a starting point for your own projects for the Computer Graphics course at Politecnico di Torino.
> Last updated: 2025-11-03

---

## Table of Contents
- [Overview](#overview)
- [Stack and Requirements](#stack-and-requirements)
- [Project Structure](#project-structure)
- [Setup](#setup)
  - [Windows (vcpkg)](#windows-vcpkg)
  - [macOS (Homebrew)](#macos-homebrew)
  - [Linux (apt)](#linux-apt)
- [Build and Run](#build-and-run)
- [Resources and Paths](#resources-and-paths)
- [Environment Variables / Defines](#environment-variables--defines)
- [Icon (Optional)](#icon-optional)
  - [Windows](#windows)
  - [macOS](#macos)

---

## Overview
This repository contains a minimal but structured OpenGL application scaffold:
- `OpenGLApp/Main.cpp` is the entry point: creates the window/OpenGL context, initializes audio, loads a font, and pushes the `MainViewController`.
- Resource loading uses a helper `getResource(...)` that relies on a compile-time path define so assets work across platforms.
- Directories for data/game/view classes are provided to help you organize your code.

You are expected to customize the app name and add your own gameplay and rendering code.

**TODO:**
- Replace the placeholder project/app names in `CMakeLists.txt` (`APP_NAME`) and in `OpenGLApp/Main.cpp` (`gameName`).

---

## Stack and Requirements
- Language: C++14 (set via `CMAKE_CXX_STANDARD 14`)
- Build system: CMake (minimum 3.10)
- Graphics: OpenGL
- Core libraries:
  - GLFW (window/context/input)
  - GLAD (OpenGL loader)
  - GLM (math) — vendored in `glm-master`
  - Assimp (model loading)
  - FreeType (text rendering)
  - OpenAL + libsndfile (audio)
- Package managers:
  - Windows: vcpkg
  - macOS: Homebrew
  - Linux: apt

Tools/IDEs: CLion, Visual Studio, Xcode, or any CMake-capable environment.

---

## Project Structure
- `CMakeLists.txt` — build configuration; links platform deps; defines `RESOURCE_PATH` and bundles/copies assets post-build.
- `OpenGLApp/`
  - `Main.cpp` — program entry point.
  - `resources/` — all runtime assets (music, sfx, shaders, fonts, icons, etc.).
  - `ViewController/` — screens/views (e.g., `MainViewController`).
  - `GameClasses/` — add your game-domain classes here.
  - `DataClasses/` — reusable components like `shader.h`, `model.h`, `SpriteRenderer`, `SoundManager`, `SoundEngine`, etc.
  - `app_icon.rc` — Windows application icon resource.
- `glad/` — GLAD headers/sources.
- `glm-master/` — GLM library (vendored).
- `Installation/` — optional installers (e.g., CMake MSI).

Notes:
- `file(GLOB_RECURSE ...)` in `CMakeLists.txt` already collects sources under `OpenGLApp/` and `glad/src`.
- Do not move core folders without updating include paths in `CMakeLists.txt`.

---

## Setup

### Windows (vcpkg)
1. Install vcpkg (skip this step if you already have it):
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   ```
2. Install dependencies (from the vcpkg directory):
   ```powershell
   .\vcpkg.exe install freetype:x64-windows assimp:x64-windows glfw3:x64-windows openal-soft:x64-windows libsndfile:x64-windows
   ```
3. Configure the project (from your project root):
   ```powershell
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/Users/<YOUR_USER>/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```
   - If `cmake` is not found, see the installer under `Installation/`.
4. Edit `CMakeLists.txt`:
   - Set your vcpkg toolchain path if different: `set(CMAKE_TOOLCHAIN_FILE "C:/.../vcpkg.cmake")`.
   - Set `APP_NAME` to your executable name.

### macOS (Homebrew)
1. Install Homebrew (skip this step if you already have it):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```
2. Install dependencies:
   ```bash
   brew install assimp glfw freetype libsndfile
   ```
3. Configure:
   ```bash
   cmake -B build
   ```
   - `CMakeLists.txt` auto-detects Apple Silicon or Intel and sets include/link directories accordingly.

### Linux (apt)
1. Install toolchain and libraries:
   ```bash
   sudo apt update && sudo apt install -y \
     build-essential cmake pkg-config \
     libgl1-mesa-dev \
     libfreetype6-dev \
     libassimp-dev \
     libglfw3-dev \
     libopenal-dev \
     libsndfile1-dev
   ```
2. Configure:
   ```bash
   cmake -B build
   ```

---

## Build and Run
From the project root after configuration:

```bash
cmake --build build
```

- Windows: run the produced `.exe` under your build directory (e.g., `cmake-build-debug` or `build`).
- macOS: a `.app` bundle is created; resources are copied into the bundle.
- Linux/Windows: resources are copied next to the executable (`resources/`).

IDE tip: You can also use your IDE’s build/run buttons; they call CMake behind the scenes.

---

## Resources and Paths
- Assets live under `OpenGLApp/resources/`.
- At runtime:
  - macOS: resources are copied to `YourApp.app/Contents/resources`
  - Windows/Linux: resources are copied next to the executable in `resources/`
- Code should load assets via:
  ```cpp
  inline std::string getResource(const std::string& relativePath);
  ```
  defined in `OpenGLApp/Main.cpp`. Example:
  ```cpp
  menuMusic = SoundManager(getResource("Music/menu.mp3"), soundEngine.volMusic, true, &soundEngine);
  ```

---

## Environment Variables / Defines
- `RESOURCE_PATH` (compile definition)
  - Set in `CMakeLists.txt` via `target_compile_definitions`.
  - macOS: `RESOURCE_PATH="../resources"`
  - Windows/Linux: `RESOURCE_PATH="resources"`
  - Used by `getResource(...)` to build full asset paths at runtime.

---


## Icon (Optional)
### Windows
1. Convert `OpenGLApp/resources/Icon/Icon.png` to `Icon.ico` (e.g., using an online converter).
2. Save as `OpenGLApp/resources/Icon/Icon.ico` or update the path in `OpenGLApp/app_icon.rc`.

### macOS
Create a `.icns` from your PNG and place it in `OpenGLApp/resources/Icon/Icon.icns`:
```bash
mkdir -p OpenGLApp/resources/Icon/Icon.iconset
cp OpenGLApp/resources/Icon/Icon.png OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png
sips -z 16 16     OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/resources/Icon/Icon.iconset/icon_16x16.png
sips -z 32 32     OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/resources/Icon/Icon.iconset/icon_16x16@2x.png
sips -z 128 128   OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/resources/Icon/Icon.iconset/icon_128x128.png
sips -z 256 256   OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/resources/Icon/Icon.iconset/icon_128x128@2x.png
iconutil -c icns OpenGLApp/resources/Icon/Icon.iconset
```
`CMakeLists.txt` sets the macOS bundle icon to `Icon.icns`.

---
