# Table of Contents
- [Installation](#installation)
    - [On Windows](#on-windows)
    - [On macOS](#on-macos)
- [Usage](#usage)
    - [Project structure](#project-structure)
    - [Loading Resources](#loading-resources)
    - [Icon (Optional)](#icon-optional)
        - [On Windows](#on-windows-1)
        - [On macOS](#on-macos-1)
- [Build and Run](#build-and-run)

# Installation
#### On Windows

When opening the project for the first time, select the `Visual Studio` toolchain and not `MinGW`.

Install vcpkg if you don't have it on your Windows system pasting this line below in the terminal:
```bash
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```
Then install the dependencies (you should be in the vcpkg folder)
```bash
.\vcpkg.exe install freetype:x64-windows assimp:x64-windows glfw3:x64-windows openal-soft:x64-windows libsndfile:x64-windows
```
**ATTENTION**: It could take some minutes.

After the installation, in your **Project Folder**, run:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE= your_vcpkg_path+"scripts/buildsystems/vcpkg.cmake"
cmake --build build
```
**IMPORTANT**: If the cmake command is not found, in the [Installation](Installation/cmake-4.2.0-rc1-windows-x86_64.msi) folder, there is the `.msi` file to install it.

#### On macOS 
You need Homebrew to install dependencies.
If you don't have it, install it by running this command in the terminal:
```bash
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
To check if everything is correct, run (after reopening the terminal):
```bash
  brew --version
```
If the installation was successful it should print something like: `Homebrew 4.x.x`

Use brew to install dependencies:
```bash
  brew install assimp glfw freetype libsndfile
```
To Change your App Name, go to the [CmakeLists.txt](CMakeLists.txt) at line 8 and change the name of the project.
And in the [Main](OpenGLApp/Main.cpp) at line 48.

# Usage
## Project structure

The project is structured in the following way:
- [CMakeLists.txt](CMakeLists.txt)
- [glad](glad)
- [glm-master](glm-master)
- [Installation](Installation)
- [OpenGLApp](OpenGLApp)
  - [Main.cpp](OpenGLApp/Main.cpp)
  - [app_icon.rc](OpenGLApp/app_icon.rc)
  - [resources](OpenGLApp/resources)
  - [ViewController](OpenGLApp/ViewController)
  - [Game Classes](OpenGLApp/GameClasses)
  - [Data Classes](OpenGLApp/DataClasses)

The Main.cpp file is the entry point of the project. It creates the window, the OpenGL context, and some global variables.

From here, it loads the [Main View Controller](OpenGLApp/ViewController/MainViewController.cpp).
This is the first screen of the game. Right now there is a template background and a button to start the game (Not implemented yet).

In the [Game Classes](OpenGLApp/GameClasses) folder (_now empty_), you can add your own classes for the game, such as Player, GameManager, etc.

In the [Data Classes](OpenGLApp/DataClasses) folder, there are some classes used to load resources, such as the [shader.h](OpenGLApp/DataClasses/shader.h).
In addition, here, there are:
- [Button class](OpenGLApp/DataClasses/Button.h) 
- [Position class](OpenGLApp/DataClasses/Position.h) used to manage the position of the objects.
- [Sound Manager](OpenGLApp/DataClasses/SoundManager.h) and [Sound Engine](OpenGLApp/DataClasses/SoundEngine.h) used to load and play sounds.

**IMPORTANT**: You can change this structure, but do not move the _CMakeLists.txt_, _Main.cpp_, _app_icon.rc_, the _resources_' folder, the _glad_ and _glm-master_ folders. 
If you do, you will have to change the path in the CMakeLists.txt file.

## Loading Resources

To make the project work, keep the directories struct of `glad` and `glm-master` in the root directory.
Then in the `OpenGLApp/OpenGLApp` you can find your `Main.cpp` file and in this directory you can add all your costume files.

**IMPORTANT**: The [resources](OpenGLApp/resources) directory must be in the same directory as the [Main.cpp](OpenGLApp/Main.cpp) file.
To load a resource, you have to add `../` before the path: `../resources/your_object.obj` and not `resources/your_object.obj`.

If you want to change this structure, you can change the [CmakeLists.txt](CMakeLists.txt) file.

### Icon (Optional)
#### On Windows
Convert the [Icon file](OpenGLApp/resources/Icon/Icon.png) to a file .ico.
You can use [this](https://convertio.co/es/png-ico/) website to do it.

Remember to save the .ico file in the [Icon](OpenGLApp/resources/Icon) directory. If not, you have to change the path in the [app_icon.rc](OpenGLApp/app_icon.rc)

#### On macOS
To use a costume icon, create a file named icon.png in the resources' directory.
Then in the terminal, from the project directory, run:
```bash
    mkdir -p OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset
    cp OpenGLApp/OpenGLApp/resources/Icon/Icon.png OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png
    sips -z 16 16     OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_16x16.png
    sips -z 32 32     OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_16x16@2x.png
    sips -z 128 128   OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_128x128.png
    sips -z 256 256   OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_512x512.png --out OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset/icon_128x128@2x.png
    iconutil -c icns OpenGLApp/OpenGLApp/resources/Icon/Icon.iconset
```
If everything went well, you should have a file named `Icon.icns` in the [here](OpenGLApp/resources/Icon/Icon.icns).

# Build and Run
Once you have built the project, it will create a folder named `cmake-build-debug` on macOS, and `cmake-build-debug-visual-studio` in the root directory.
In this folder, you can run the executable file with your APP name.

You can also execute it directly from the IDE.