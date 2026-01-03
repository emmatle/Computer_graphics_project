//
// Created by Emma Toutel on 03/01/2026.
//



#ifndef ESCAPE_ROOM_TEXT_RENDERER_H
#define ESCAPE_ROOM_TEXT_RENDERER_H

#include <string>
#include <glm/glm.hpp>
#include <map>




struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

extern std::map<char, Character> Characters;
extern unsigned int textVAO;       // VAO for text
extern unsigned int textVBO;       // VBO for text
extern unsigned int shaderProgram; // shader for text

extern unsigned int textVAO3D;
extern unsigned int textVBO3D;
extern unsigned int shaderProgram3D;

bool InitTextRenderer(int SCR_WIDTH, int SCR_HEIGHT);

void RenderText(std::string text, float x, float y, float scale, glm::vec3 color);

void RenderText3D(
    const std::string& text,
    const glm::vec3& position,
    const glm::vec3& right,
    const glm::vec3& up,
    float scale,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& color
);

#endif //ESCAPE_ROOM_TEXT_RENDERER_H

