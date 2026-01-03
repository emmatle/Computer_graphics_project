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

class Mesh {
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
public:
    enum Type { // Used for special cases
        Default,
        Collision,
        Button,
        Light,
        Portal
    } type;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Texture *textures[Texture::N_TYPES] = {}; // Each mesh can use only one texture per type

    std::string name;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string name = "", Type type = Default) :
            vertices(std::move(vertices)),
            indices(std::move(indices)),
            name(std::move(name)),
            type(type) {
        init();
    }

    void free() const {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    // Render the mesh skip textures settings for the picking shader
    void draw(Shader &shader, bool useTextures = true) const {
        if (useTextures) applyTextures(shader);

        // Draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

        // Reset to default
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

        // Vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);
        // Vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));
        // Vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, texCoords));
        // Vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, tangent));

        glBindVertexArray(0);
    }

    void applyTextures(const Shader &shader) const {
        const char* samplers[] = {
            "diffuseMap",
            "ambientOcclusionMap",
            "normalMap",
            "roughnessMap",
            "metalnessMap",
        };
        const char* flags[] = {
            "hasDiffuseMap",
            "hasAmbientOcclusionMap",
            "hasNormalMap",
            "hasRoughnessMap",
            "hasMetalnessMap",
        };

        // For simplicity each texture unit is bound to the corresponding type enum
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