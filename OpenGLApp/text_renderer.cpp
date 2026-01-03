#include "text_renderer.h"
#include "utils.h"
#include "shader.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Character {
    unsigned int textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

// Map for extended ASCII
Character characters[256] = {};

unsigned int textVAO = 0, textVBO = 0;
unsigned int textVAO3D = 0, textVBO3D = 0;

// Shaders initialized as null; compiled in init
Shader* textShader = nullptr;
Shader* text3DShader = nullptr;

// ---------------------- HELPERS ----------------------

// Converts UTF-8 (multi-byte) to Latin-1 (single-byte 0-255)
std::string convertUTF8toLatin1(const std::string& input) {
    std::string output;
    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) {
            output += static_cast<char>(c);
        } else if ((c & 0xE0) == 0xC0 && i + 1 < input.length()) {
            unsigned char next = static_cast<unsigned char>(input[++i]);
            unsigned char val = static_cast<unsigned char>(((c & 0x03) << 6) | (next & 0x3F));
            output += static_cast<char>(val);
        }
    }
    return output;
}

bool initTextRenderer(std::string fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR: FreeType: Could not init FreeType Library" << std::endl;
        return false;
    }

    fontPath = getResourcePath(fontPath);
    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cout << "ERROR: FreeType: Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Use int loop to avoid unsigned char overflow (infinite loop)
    for (int i = 0; i < 256; i++) {
        if (FT_Load_Char(face, static_cast<unsigned long>(i), FT_LOAD_RENDER)) {
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                     static_cast<GLsizei>(face->glyph->bitmap.width),
                     static_cast<GLsizei>(face->glyph->bitmap.rows),
                     0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        characters[i] = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Initialize Shaders
    textShader = new Shader("shaders/Text.glsl");
    text3DShader = new Shader("shaders/Text3D.glsl");
    if (!textShader->compile() || !text3DShader->compile()) return false;

    // 2D Quads
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    // 3D Quads
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
    return true;
}

void renderText(const std::string &text, float x, float y, float scale, glm::vec3 color) {
    std::string cleanText = convertUTF8toLatin1(text);

    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(fbWidth), static_cast<float>(fbHeight), 0.0f);
    textShader->use();
    textShader->set("textColor", color);
    textShader->set("projection", proj);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST); // Usually want 2D text on top of everything

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (unsigned char c : cleanText) {
        const Character &ch = characters[c];
        if (ch.textureID == 0) {
            x += static_cast<float>(ch.advance >> 6) * scale;
            continue;
        }

        // xpos: baseline + horizontal bearing
        // ypos: baseline - vertical bearing (moves top of quad UP)
        float xpos = x + static_cast<float>(ch.bearing.x) * scale;
        float ypos = y - static_cast<float>(ch.bearing.y) * scale;
        float w = static_cast<float>(ch.size.x) * scale;
        float h = static_cast<float>(ch.size.y) * scale;

        float vertices[6][4] = {
            {xpos,     ypos + h,   0.0f, 1.0f},
            {xpos,     ypos,       0.0f, 0.0f},
            {xpos + w, ypos,       1.0f, 0.0f},
            {xpos,     ypos + h,   0.0f, 1.0f},
            {xpos + w, ypos,       1.0f, 0.0f},
            {xpos + w, ypos + h,   1.0f, 1.0f}
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += static_cast<float>(ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void renderText3D(const std::string &text, const glm::vec3 &position, const glm::vec3 &right, const glm::vec3 &up,
                  float scale, const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &color) {
    std::string cleanText = convertUTF8toLatin1(text);

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(right, 0.0f);
    model[1] = glm::vec4(up, 0.0f);
    model[2] = glm::vec4(glm::cross(right, up), 0.0f);
    model[3] = glm::vec4(position, 1.0f);

    text3DShader->use();
    text3DShader->set("textColor", color);
    text3DShader->set("model", model);
    text3DShader->set("view", view);
    text3DShader->set("projection", projection);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(textVAO3D);

    float penX = 0.0f;
    for (unsigned char c : cleanText) {
        const Character &ch = characters[c];
        if (ch.textureID == 0) {
            penX += static_cast<float>(ch.advance >> 6) * scale;
            continue;
        }

        float x0 = penX + static_cast<float>(ch.bearing.x) * scale;
        float y_top = static_cast<float>(ch.bearing.y) * scale;
        float y_bot = static_cast<float>(ch.bearing.y - ch.size.y) * scale;
        float w = static_cast<float>(ch.size.x) * scale;

        float vertices[6][5] = {
            {x0,     y_bot, 0.0f, 0.0f, 1.0f},
            {x0,     y_top, 0.0f, 0.0f, 0.0f},
            {x0 + w, y_top, 0.0f, 1.0f, 0.0f},
            {x0,     y_bot, 0.0f, 0.0f, 1.0f},
            {x0 + w, y_top, 0.0f, 1.0f, 0.0f},
            {x0 + w, y_bot, 0.0f, 1.0f, 1.0f}
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO3D);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        penX += static_cast<float>(ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
}

void freeTextRenderer() {
    for (int i = 0; i < 256; i++) {
        if (characters[i].textureID != 0) glDeleteTextures(1, &characters[i].textureID);
    }
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteVertexArrays(1, &textVAO3D);
    glDeleteBuffers(1, &textVBO3D);

    delete textShader;
    delete text3DShader;
}