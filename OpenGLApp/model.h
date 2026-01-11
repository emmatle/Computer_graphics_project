#pragma once

#include "utils.h"
#include "texture.h"
#include "shader.h"
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
};

struct Material {
    std::string name;
    glm::vec3 k_a = glm::vec3(1.0f);
    glm::vec3 k_d = glm::vec3(1.0f);
    glm::vec3 k_s = glm::vec3(0.5f);
    int illum = 2;
    float shininess = 32.0f;
    Texture *textures[Texture::N_TYPES] = {};

    void apply(const Shader &shader) const {
        const char *samplers[] = {"diffuseMap", "ambientOcclusionMap", "normalMap", "roughnessMap", "metalnessMap"};
        const char *flags[] = {
            "hasDiffuseMap", "hasAmbientOcclusionMap", "hasNormalMap", "hasRoughnessMap", "hasMetalnessMap"
        };
        // shader.set("k_a", k_a); // Ambient term is not used for simplicity
        shader.set("k_d", k_d);
        shader.set("k_s", k_s);
        shader.set("illum", illum);
        shader.set("shininess", shininess);

        for (int unit = 0; unit < Texture::N_TYPES; unit++) {
            if (textures[unit]) {
                textures[unit]->use(unit);
                shader.set(samplers[unit], unit);
                shader.set(flags[unit], true);
            } else {
                shader.set(flags[unit], false);
            }
        }
    }
};

class Mesh {
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // AABB
    glm::vec3 min, max;

    Material *material;
    std::string name;

    Mesh(std::vector<Vertex> v, std::vector<unsigned int> i, Material *mat, std::string n = "",
         const glm::vec3 &min = glm::vec3(FLT_MAX), const glm::vec3 &max = glm::vec3(-FLT_MAX))
        : vertices(std::move(v)), indices(std::move(i)), material(mat), name(std::move(n)), min(min), max(max) {
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


class Model {
    IAssetManager *assetManager;
    std::vector<Material> materials;

public:
    std::vector<Mesh> meshes;
    std::vector<AABB> collisions;
    std::string path;

    Model(IAssetManager *am, const std::string &path = "") : assetManager(am), path(path) {
    }

    bool load() {
        if (!assetManager) {
            std::cerr << "ERROR: assetManager is nullptr" << std::endl;
            return false;
        }
        if (path.empty()) {
            std::cerr << "ERROR: model path is empty" << std::endl;
        }
        std::string fullPath = getResourcePath(path);

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            fullPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR: ASSIMP: " << importer.GetErrorString() << std::endl;
            return false;
        }

        // Preload all materials
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial *aiMat = scene->mMaterials[i];
            Material mat;
            mat.name = aiMat->GetName().C_Str();
            int shadingModel = 2; // Defaults to Blinn-Phong
            if (aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == AI_SUCCESS) {
                if (shadingModel == aiShadingMode_NoShading) {
                    mat.illum = 0;
                } else if (shadingModel == aiShadingMode_Flat) {
                    mat.illum = 1;
                }
            }
            aiColor3D ambient(1.0f, 1.f, 1.0f);
            aiColor3D diffuse(1.f, 1.f, 1.f);
            aiColor3D specular(0.5f, 0.5f, 0.5f);

            if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_AMBIENT, ambient))
                mat.k_a = glm::vec3(ambient.r, ambient.g, ambient.b);

            if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
                mat.k_d = glm::vec3(diffuse.r, diffuse.g, diffuse.b);

            if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_SPECULAR, specular))
                mat.k_s = glm::vec3(specular.r, specular.g, specular.b);

            float shininess = 32.0f;
            if (AI_SUCCESS == aiMat->Get(AI_MATKEY_SHININESS, shininess)) {
            }

            loadMaterialTextures(aiMat, mat);
            materials.push_back(mat);
        }

        // Process mesh hierarchy
        processNode(scene->mRootNode, scene);
        return true;
    }

    void free() { for (auto &mesh: meshes) mesh.free(); }

    void draw(Shader &shader, bool useMaterial = true) const {
        for (const auto &mesh: meshes) mesh.draw(shader, useMaterial);
    }

private:
    void processNode(aiNode *node, const aiScene *scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            processMesh(scene->mMeshes[node->mMeshes[i]]);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) processNode(node->mChildren[i], scene);
    }

    void processMesh(aiMesh *mesh) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        glm::vec3 min(FLT_MAX), max(-FLT_MAX);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex v = {};
            v.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

            min = glm::min(min, v.position);
            max = glm::max(max, v.position);

            if (mesh->HasNormals()) v.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
            if (mesh->mTextureCoords[0]) {
                v.texCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
                v.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
            }
            vertices.push_back(v);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);
        }

        std::string name = mesh->mName.C_Str();
        std::string nameLow = stringToLower(name);

        if (nameLow.find("collision") != std::string::npos) {
            collisions.emplace_back(AABB{min, max}); // Don't create the mesh
            return;
        }

        // TODO: Handle the different cases.
        if (nameLow.find("glass") != std::string::npos) return; // Skip glass for now

        // Assign correct preloaded material pointer
        Material *matPtr = (mesh->mMaterialIndex < materials.size()) ? &materials[mesh->mMaterialIndex] : nullptr;
        meshes.emplace_back(vertices, indices, matPtr, name);
    }

    void loadMaterialTextures(aiMaterial *mat, Material &material) {
        aiTextureType types[] = {
            aiTextureType_DIFFUSE, aiTextureType_SPECULAR, aiTextureType_AMBIENT,
            aiTextureType_HEIGHT, aiTextureType_NORMALS, aiTextureType_SHININESS, aiTextureType_REFLECTION
        };

        for (const auto type: types) {
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                aiString str;
                mat->GetTexture(type, i, &str);
                Texture *tex = assetManager->getTexture(str.C_Str());
                if (tex) material.textures[tex->type] = tex; // Maps Assimp texture to internal type
            }
        }
    }
};
