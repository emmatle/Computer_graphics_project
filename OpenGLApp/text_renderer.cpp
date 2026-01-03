//
// Created by Emma Toutel on 03/01/2026.
//

#include "text_renderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <string>

//for import the tff file
#include <filesystem>

// in the texture part and for 3D text
#include <map>         // for storing Characters
#include <glm/glm.hpp> // for glm::ivec2

// in the rendering on the screen with shader part
#include <glad/glad.h> // for OpenGL functions
#include <GLFW/glfw3.h> // if you are using GLFW
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>




// ---------------------- STRUCT & GLOBALS ----------------------

// creating a place to store glyph data to use it easily for every rendering
//struct Character {
//    unsigned int TextureID; // ID handle of the glyph texture
//    glm::ivec2 Size;        // Size of glyph
//    glm::ivec2 Bearing;     // Offset from baseline to left/top of glyph
//    unsigned int Advance;   // Horizontal offset to advance to next glyph
//};

// Map to store all ASCII characters for fast rendering
std::map<char, Character> Characters;

// Global OpenGL objects
unsigned int textVAO= 0;       // rename VAO for text
unsigned int textVBO= 0;       // rename VBO for text
unsigned int shaderProgram= 0;

//new function for text in the 3D world
unsigned int textVAO3D = 0;
unsigned int textVBO3D = 0;
unsigned int shaderProgram3D = 0;

// ---------------------- INITIALIZATION FUNCTION ----------------------
bool InitTextRenderer(int SCR_WIDTH, int SCR_HEIGHT)
{
    // FreeType library has to be initialized
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////mac  ???
    namespace fs = std::filesystem;

    // Go up one level from build/ to project root
    fs::path fontPath = fs::current_path()
                      / ".."
                      / "resources"
                      / "fonts"
                      / "Antonio-Bold.ttf";

    fontPath = fs::weakly_canonical(fontPath);

    if (!fs::exists(fontPath))
    {
        std::cerr << "ERROR: Font not found: " << fontPath << std::endl;
        return false;
    }

    std::string font_name = fontPath.string();

    //std::string font_name = "/Users/emmatoutel/Desktop/OpenGL-Project-main_emma/OpenGLApp/resources/fonts/Antonio-Bold.ttf";

    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return false;
    }

    // Set the pixel size for the GLYPHS
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Set pixel unpack alignment for OpenGL to read 1-byte grayscale textures
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Loop through ASCII characters
    for (unsigned char c = 0; c < 128; c++) {
        // Load and render glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYPE: Failed to load Glyph " << c << std::endl;
            continue;
        }

        // Generate OpenGL texture : this part has stored every opacity (texture) for each pixel of the glyphs
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        if (face->glyph->bitmap.width == 0 || face->glyph->bitmap.rows == 0)////////////////////////debug
        {
            // still store character metrics, but skip texture creation
            Character character = {
                0,
                glm::ivec2(0, 0),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
            continue;
        }

        ///////////////////////////////////////////////////////////////////////////////////
        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_R8, //INTERNAL FORMAT (GPU storage) for mac GL_R8 important
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0, GL_RED , //SOURCE FORMAT (CPU memory)
                     GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);



        // 🔥 REQUIRED on macOS (texture swizzle)////////////////////////////////////////////////////////////??

#ifdef __APPLE__
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
#endif

        //////////////////////////////////////////////////////////////////////////////////////////////////////

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store glyph metrics
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));

        //////////////////////////////////////////////////////check if the bitmap data has been created correctly : #opacity, otherwise transparent
        // if (c == 'A') {
        //     FT_Bitmap& bmp = face->glyph->bitmap;
        //
        //     std::cout << "Glyph 'A' bitmap:\n";
        //     for (int y = 0; y < bmp.rows; ++y) {
        //         for (int x = 0; x < bmp.width; ++x) {
        //             unsigned char value = bmp.buffer[y * bmp.pitch + x];
        //             std::cout << (value > 0 ? '#' : '.');
        //         }
        //         std::cout << '\n';
        //     }
        //}


        //std::cout << "Characters loaded: " << Characters.size() << std::endl;
    }

    // Clean up FreeType resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    //////////////////////////////
    // Shader setup (directly in C++ file)
    //////////////////////////////

    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;
    uniform mat4 projection;
    void main()
    {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
    )";

    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 FragColor;

    uniform sampler2D text;
    uniform vec3 textColor;

    void main()
    {
        float alpha = texture(text, TexCoords).a; //// important for mac due to our one channel opacity
        FragColor = vec4(textColor, alpha);
    }
    )";


    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    ////////////////////////////////////////////////////////////////?
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "text"), 0);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    ////////
    // Important part for rendering in 3D game
    ////////
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create orthographic projection matrix
    // glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH),
    //                                   0.0f, static_cast<float>(SCR_HEIGHT));
///////////////////////////////////////////////////////////////////////////////////////match our screen coord
    glm::mat4 projection = glm::ortho(
    0.0f, (float)SCR_WIDTH,
    (float)SCR_HEIGHT, 0.0f);


    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));

    ////////
    // VBO and VAO for rendering quads
    ////////
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////3DSHADER
    // ===================== 3D TEXT SHADER =====================
    const char* vs3D = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTex;

    out vec2 TexCoords;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoords = aTex;
    })";

    const char* fs3D = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 FragColor;
    uniform sampler2D text;
    uniform vec3 textColor;
    void main()
    {
        float alpha = texture(text, TexCoords).a;
        FragColor = vec4(textColor, alpha);
    })";

    unsigned int v3 = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v3, 1, &vs3D, NULL);
    glCompileShader(v3);

    unsigned int f3 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f3, 1, &fs3D, NULL);
    glCompileShader(f3);

    shaderProgram3D = glCreateProgram();
    glAttachShader(shaderProgram3D, v3);
    glAttachShader(shaderProgram3D, f3);
    glLinkProgram(shaderProgram3D);

    glDeleteShader(v3);
    glDeleteShader(f3);

    glGenVertexArrays(1, &textVAO3D);
    glGenBuffers(1, &textVBO3D);

    glBindVertexArray(textVAO3D);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO3D);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    return true;
}




// ------------------ RenderText Function ------------------
void RenderText(std::string text, float x, float y, float scale, glm::vec3 color)
{
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.x, color.y, color.z);

    // Make sure the shader samples from texture unit 0////////////////////////////////////////////////////////////????
    glUniform1i(glGetUniformLocation(shaderProgram, "text"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (char c : text)
    {
        Character ch = Characters[c];

        // Skip glyphs with no texture//////////////debug
        if (ch.TextureID == 0)
        {
            x += (ch.Advance >> 6) * scale;
            continue;
        }

        float xpos = x + ch.Bearing.x * scale;
        //float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float ypos = y - ch.Bearing.y * scale;


        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 0.0f },

            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f }
        };


        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
/////////////////////////////////////////////////////?
        glActiveTexture(GL_TEXTURE0);


        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////new function
void RenderText3D(
    const std::string& text,
    const glm::vec3& position,
    const glm::vec3& right,
    const glm::vec3& up,
    float scale,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& color
)
{
    glUseProgram(shaderProgram3D);
    glUniform3fv(glGetUniformLocation(shaderProgram3D, "textColor"), 1, &color[0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Build the model matrix
    glm::mat4 model(1.0f);
    model[0] = glm::vec4(right, 0.0f);         // X-axis
    model[1] = glm::vec4(-up, 0.0f);           // Y-axis flipped to make letters upright
    model[2] = glm::vec4(glm::cross(right, -up), 0.0f); // Z-axis
    model[3] = glm::vec4(position, 1.0f);      // translation

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram3D, "model"),
                       1, GL_FALSE, glm::value_ptr(model));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO3D);

    float penX = 0.0f;

    for (char c : text)
    {
        Character ch = Characters[c];

        if (ch.TextureID == 0) {
            penX += (ch.Advance >> 6) * scale;
            continue;
        }

        // Vertex positions for the quad
        float x0 = penX + ch.Bearing.x * scale;
        float y0 = ch.Bearing.y * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][5] = {
            { x0,     y0 + h, 0.0f, 0.0f, 1.0f },
            { x0,     y0,     0.0f, 0.0f, 0.0f },
            { x0 + w, y0,     0.0f, 1.0f, 0.0f },

            { x0,     y0 + h, 0.0f, 0.0f, 1.0f },
            { x0 + w, y0,     0.0f, 1.0f, 0.0f },
            { x0 + w, y0 + h, 0.0f, 1.0f, 1.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO3D);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        penX += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
