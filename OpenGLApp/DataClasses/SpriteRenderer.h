#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "Texture2D.h"

class SpriteRenderer
{
public:
    SpriteRenderer();
    SpriteRenderer(Shader& shader);
    ~SpriteRenderer();

    void DrawSprite(Texture2D& texture, glm::vec2 position,
        glm::vec2 size = glm::vec2(10.0f, 10.0f), float rotate = 0.0f,
        glm::vec3 color = glm::vec3(1.0f));

    void DrawSpriteSlider(Texture2D& texture, glm::vec2 position,
        glm::vec2 size = glm::vec2(10.0f, 10.0f), float rotate = 0.0f,
        glm::vec3 color = glm::vec3(1.0f), float maxWidth = 0.0f);

    SpriteRenderer& operator=(SpriteRenderer op);
private:
    Shader       shader;
    unsigned int quadVAO;

    void initRenderData();
};

