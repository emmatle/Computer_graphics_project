#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"
#include "texture.h"

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    int boneIds[MAX_BONE_INFLUENCE];
    float weights[MAX_BONE_INFLUENCE];
};

class Mesh {
    unsigned int VBO, EBO; // Render Data
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
    std::vector<Texture> textures;
    unsigned int VAO;
    std::string name;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures = {},
         std::string name = "", Type type = Default) :
            vertices(std::move(vertices)),
            indices(std::move(indices)),
            textures(std::move(textures)),
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
    void draw(const Shader &shader, bool useTextures = true) const {
        static bool silenceWarning = false;
        bool hasDiffuse = false;
        bool hasNormal = false;
        bool hasRoughness = false;

        if (useTextures) {
            if (textures.empty()) {
                if (!silenceWarning) {
                    std::cerr << "WARNING: undefined texture(s)" << std::endl;
                    silenceWarning = true;
                }
            }
            for (int i = 0; i < textures.size(); i++) {
                if (textures[i].type == Texture::Diffuse) hasDiffuse = true;
                if (textures[i].type == Texture::Normal) hasNormal = true;
                if (textures[i].type == Texture::Roughness) hasRoughness = true;
                textures[i].use(i);
                shader.set(textures[i], i);
            }
            shader.set("hasDiffuse", hasDiffuse);
            shader.set("hasNormal", hasNormal);
            shader.set("hasRoughness", hasRoughness);
        }


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
        // Vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, bitangent));
        // Bone IDs
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void *) offsetof(Vertex, boneIds));
        // Weights
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, weights));

        glBindVertexArray(0);
    }
};