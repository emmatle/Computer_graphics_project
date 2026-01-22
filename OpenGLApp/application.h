#pragma once

#include <string>
#include <SFML/Audio.hpp>

class Game;
class Renderer;
struct GLFWwindow;

class Application {
public:
    static GLFWwindow *window;
    static std::string title;
    static int width;
    static int height;

    static bool fullscreen;
    static bool vsync;
    static float mouseSensitivity;
    static float fontSize;

    static void setBackgroundLowpass(bool enable);

    void run();

private:
    static Game game;
    static Renderer renderer;
    static float mouseX;
    static float mouseY;
    static bool gameFocus;
    static bool firstMouse;
    static float time;
    static float fps;

    static sf::Music backgroundMusic;
    static sf::Music backgroundMusicLowpass;
    static bool backgroundLowpassActive;

    void init();

    void terminate();

    static void updateTime();

    void loadSettings();

    void storeSettings();

    static void drawDebugMenu();

    // --- Callbacks ---

    static void windowCloseCallback(GLFWwindow *window);

    static void windowSizeCallback(GLFWwindow *window, int width, int height);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    static void charCallback(GLFWwindow *window, unsigned int codepoint);
};