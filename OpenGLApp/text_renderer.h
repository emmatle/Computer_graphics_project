#pragma once

#include <string>
#include <glm/glm.hpp>

bool initTextRenderer(std::string fontPath);

void renderText(const std::string &text, float x, float y, float scale, glm::vec3 color);

void renderText3D(const std::string &text, const glm::vec3 &position, const glm::vec3 &right, const glm::vec3 &up,
                  float scale, const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &color
);

void freeTextRenderer();