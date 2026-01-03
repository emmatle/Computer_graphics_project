#pragma once

#include "shader.h"
#include <glm/glm.hpp>

class Object;

class Light {
public:
    glm::vec3 position;
    glm::vec3 color;
    float intensity;

    // Attenuation constants for Point Lights
    float constant;
    float linear;
    float quadratic;

    Object *parent;

    // Constructor passes position to the Object base class
    Light(const glm::vec3 &pos = {}, const glm::vec3 color = glm::vec3(1.f), const float intensity = 1.f,
          const float constant = 0.75f, const float linear = 0.5f, const float quadratic = 0.25f) : position(pos),
        color(color), intensity(intensity), constant(constant), linear(linear), quadratic(quadratic) {
    }

    void apply(const Shader &shader, int index) const {
        std::string base = "lights[" + std::to_string(index) + "]";

        shader.set((base + ".position").c_str(), position);
        shader.set((base + ".color").c_str(), color * intensity);
        shader.set((base + ".constant").c_str(), constant);
        shader.set((base + ".linear").c_str(), linear);
        shader.set((base + ".quadratic").c_str(), quadratic);
    }
};
