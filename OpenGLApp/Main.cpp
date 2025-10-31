#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <string>

#include "DataClasses/Character.h"
#include "ViewController/MainViewController.h"
#include "DataClasses/SpriteRenderer.h"
#include "DataClasses/SoundManager.h"
#include "DataClasses/SoundEngine.h"
#include "DataClasses/shader.h"
#include "DataClasses/model.h"

#include <ft2build.h>
#include FT_FREETYPE_H

// Screen Size (Default)
int SCR_WIDTH = 1920;
int SCR_HEIGHT = 1080;

// Shaders
Shader gameShader; //Used for 3D Objects
Shader shaderText; //Used for Text Rendering
Shader menuBGShader; //Used for the background

SpriteRenderer sprite2D; //Used for 2D Objects, such as images
glm::mat4 model;

// Camera Position
float lastX = static_cast<float>(SCR_WIDTH) / 2.0f;
float lastY = static_cast<float>(SCR_HEIGHT) / 2.0f;

// Timers
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Text
std::map<GLchar, Character> Characters;
unsigned int VAOText, VBOText;

//TODO: Change this to your game Name, so the new Window will have this name
string gameName = "YOUR GAME";

// Sound
SoundManager menuMusic;
SoundManager sfxMouse;
SoundEngine soundEngine;

// Those variables are used to fix the window to the screen size
float scaleScreen;
float minHeight;
float maxHeight;
float maxWidth;

// Function to get the screen size
void getScreenSize(int& screenWidth, int& screenHeight);

// Function to create the window
GLFWwindow* setWindow();

// Function to load the font type
int loadFont(const std::string& font_name);


Button startButton;

int main()
{
        getScreenSize(SCR_WIDTH, SCR_HEIGHT);
        scaleScreen = 1.0f * static_cast<float>(SCR_HEIGHT) / 1600;

        maxHeight = static_cast<float>(SCR_HEIGHT) / 2.0f * 95/100;
        maxWidth = static_cast<float>(SCR_WIDTH) / 2.0f * 95/100;
        minHeight = static_cast<float>(SCR_HEIGHT) / 2.0f*77/100;

        model = glm::mat4(1.0f);
        
        //----Set SoundEngine
        soundEngine.setVolSuono(100);
        soundEngine.setVolMusica(100);
        soundEngine.startEngine();

        //----Set Music
        menuMusic = SoundManager("../resources/Music/menu.mp3", soundEngine.volMusic, true, &soundEngine);
        sfxMouse = SoundManager("../resources/SFX/mouseClick.wav", soundEngine.volMusic, false, &soundEngine);

        //----Opening Window
        GLFWwindow* window = setWindow();

        //----Load Font
        loadFont("../resources/Fonts/Antonio/static/Antonio-Bold.ttf");

        //----Start the Main View
        MainViewController mainView;
        mainView.main(window);
        glfwSetWindowShouldClose(window, true);
        return 0;
}

void getScreenSize(int& screenWidth, int& screenHeight) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Impossibile inizializzare GLFW" << std::endl;
        return;
    }

    // get the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // Get the primary Monitor mode
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // Get screen size
    screenWidth = mode->width;
    screenHeight = mode->height;

    // Terminate GLFW
    glfwTerminate();
}

GLFWwindow* setWindow() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWmonitor* monitor = glfwGetPrimaryMonitor(); // Mettere Null se non si vuole il full screen
    //glfwGetMonitorPhysicalSize(monitor, &SCR_WIDTH, &SCR_HEIGHT);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, gameName.c_str(), monitor, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    //glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    // tell stb_image.h to flip loaded texture on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    // Blending function for Alpha channels
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // configure global opengl state
    // -----------------------------
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_ALPHA);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    return window;
}

int loadFont(const std::string& font_name) {
    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different from 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // find the path to font
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    // set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // load first 128 characters of an ASCII set
    for (unsigned char c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &VAOText);
    glGenBuffers(1, &VBOText);
    glBindVertexArray(VAOText);
    glBindBuffer(GL_ARRAY_BUFFER, VBOText);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return 0;
}

float mouseX = 0;
float mouseY = 0;

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ) {
        if (startButton.selected == true) { // Check if the mouse is inside the button
            if (startButton.clicked == false) {// check if is already clicked
                sfxMouse.playSound();
                startButton.clicked = true;
            }
        }
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE ) {
        if (startButton.selected == true) {
            startButton.clicked = false;
        }
    }


    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS){

    }
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {

    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {

    }
}

// glfw: whenever the window size changed (by OS or user resize), this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    //test
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xPosIn, double yPosIn)
{
    int width, height;
    glfwGetWindowSize(window,&width, &height);
    const auto w_conv = static_cast<float>(SCR_WIDTH); //1440
    const auto h_conv = static_cast<float>(SCR_HEIGHT);  //1440
    const float xPos = (w_conv * static_cast<float>(xPosIn) / static_cast<float>(width));
    float yPos = (h_conv * static_cast<float>(yPosIn) / static_cast<float>(height));
    if (xPos > startButton.x && xPos < startButton.x + startButton.width &&
        yPos > startButton.y && yPos < startButton.y + startButton.height) {
        startButton.selected = true;
        }
    else {
        startButton.selected = false;
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
}

void RenderText(const Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate the corresponding render state
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAOText);

    // iterate through all characters
    for (std::string::const_iterator c = text.begin(); c != text.end(); ++c)
    {
        const Character ch = Characters[*c];

        float xpos = x + static_cast<float>(ch.Bearing.x) * scale;
        float ypos = y - static_cast<float>(ch.Size.y - ch.Bearing.y) * scale;

        float w = static_cast<float>(ch.Size.x) * scale;
        float h = static_cast<float>(ch.Size.y) * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBOText);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for the next glyph (note that advance is the number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide number of 1/64th pixels by 64 to get number of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D loadTextureFromFile(const char* file, bool alpha)
{
    // create texture object
    Texture2D texture;
    if (alpha)
    {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }
    // load image
    int width, height, nrChannels;
    unsigned char* data = stbi_load(file, &width, &height, &nrChannels, 0);
    
    // now generate texture
    texture.Generate(width, height, data);
    // and finally free image data
    stbi_image_free(data);
    return texture;
}
