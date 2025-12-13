#pragma once

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION // macOS 10.14 - OpenGL API deprecated.
#endif

#ifndef RESOURCE_PATH // Already defined in the CMakeLists.txt, but kept as fallback
#ifdef ___APPLE__
#define RESOURCE_PATH "../Resources"
#else
#define RESOURCE_PATH "resources"
#endif
#endif

#include <iostream>
#include <filesystem>

#include <GLFW/glfw3.h>

// Helper function for retrieving correct path for game assets
std::filesystem::path getResource(const std::string &relative, bool mute = false) {
    std::filesystem::path file = std::string(RESOURCE_PATH) + "/" + relative;
    if (!mute && !exists(file)) {
        std::cerr << "ERROR: file " << file << "not found" << std::endl;
    }
    return file;
}

#include "game.h"
#include "renderer.h"

class Application {
public:
    static GLFWwindow *window;
    static std::string title;
    static int width;
    static int height;
    static int fbWidth;
    static int fbHeight;
    static float aspect;

    static bool fullscreen;
    static bool vsync;
    static float mouseSensitivity;
    static float fontSize;

    static float currentTime;
    static float deltaTime;
    static float lastTime;

    void run();

private:
    static Game game;
    static Renderer renderer;
    static float mouseX;
    static float mouseY;
    static bool debug;
    static bool gameFocus;
    static bool firstMouse;

    void init();

    void terminate();

    void loadSettings();

    void storeSettings();

    static void loadState(int slot = 1);

    static void saveState(int slot = 1);

    static void drawDebugMenu();

    // --- Callbacks ---

    static void windowCloseCallback(GLFWwindow *window);

    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
};