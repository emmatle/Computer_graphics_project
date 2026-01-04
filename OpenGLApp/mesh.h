#pragma once

#include "texture.h"
#include "shader.h"
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
};

struct Material {
    std::string name;
    Texture *textures[Texture::N_TYPES] = {};

    void apply(const Shader &shader) const {
        const char *samplers[] = {"diffuseMap", "ambientOcclusionMap", "normalMap", "roughnessMap", "metalnessMap"};
        const char *flags[] = {
            "hasDiffuseMap", "hasAmbientOcclusionMap", "hasNormalMap", "hasRoughnessMap", "hasMetalnessMap"
        };

        for (int unit = 0; unit < Texture::N_TYPES; unit++) {
            if (textures[unit]) {
                textures[unit]->use(unit);
                shader.setInt(samplers[unit], unit);
                shader.setInt(flags[unit], true);
            } else {
                shader.setInt(flags[unit], false);
            }
        }
    }
};

class Mesh {
    unsigned int VAO, VBO, EBO;

public:
    enum Type { Default, Collision, Button, Light, Portal } type;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material *material;
    std::string name;

    Mesh(std::vector<Vertex> v, std::vector<unsigned int> i, Material *mat, std::string n = "",
         Type t = Default) : vertices(std::move(v)), indices(std::move(i)), material(mat), name(std::move(n)), type(t) {
        init();
    }

    void free() const {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    void draw(Shader &shader, bool useMaterial = true) const {
        if (useMaterial && material) material->apply(shader);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }

private:
    void init() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, texCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, tangent));
        glBindVertexArray(0);
    }
};
