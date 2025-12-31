#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <utility>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"
#include "texture.h"

class Model {
    struct Bound {
        glm::vec3 min;
        glm::vec3 max;
    };

public:
    std::vector<Mesh> meshes;
    std::vector<Bound> bounds; // Used for detecting collisions
    std::vector<Texture> loadedTextures; // Stores loaded textures to prevent duplicates
    std::filesystem::path path;

    Model(std::filesystem::path path = "") : path(std::move(path)) {
    }

    // Loads a model using ASSIMP
    bool load() {
        std::filesystem::path fullPath = getResource(path, true);
        if (!exists(fullPath)) {
            std::cerr << "ERROR: failed to load model " << fullPath << std::endl;
            return false;
        }

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(fullPath.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR: ASSIMP:\n" << importer.GetErrorString() << std::endl;
            return false;
        }

        processNode(scene->mRootNode, scene);

        return true;
    }

    // Free GPU resources
    void free() {
        for (auto &mesh: meshes) mesh.free();
        for (auto &tex: loadedTextures) tex.free();
    }

    // Draws the model by iterating through all its meshes
    void draw(const Shader &shader, bool useTextures = true) const {
        for (auto &mesh: meshes) mesh.draw(shader, useTextures);
    }

private:
    // Recursively processes nodes
    void processNode(aiNode *node, const aiScene *scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    void processMesh(aiMesh *mesh, const aiScene *scene) {
        std::string name = mesh->mName.C_Str();

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Used for bounding volumes
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(-std::numeric_limits<float>::max());

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

            if (min.x > vertex.position.x) min.x = vertex.position.x;
            if (min.y > vertex.position.y) min.y = vertex.position.y;
            if (min.z > vertex.position.z) min.z = vertex.position.z;

            if (max.x < vertex.position.x) max.x = vertex.position.x;
            if (max.y < vertex.position.y) max.y = vertex.position.y;
            if (max.z < vertex.position.z) max.z = vertex.position.z;

            if (mesh->HasNormals())
                vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

            if (mesh->mTextureCoords[0]) {
                vertex.texCoords = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
                vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
                vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
            } else { // Fallback values
                vertex.texCoords = glm::vec2(0.0f, 0.0f);
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        size_t pos;
        std::string num = "0";
        Mesh::Type type = Mesh::Default;

        // Convert to lowercase for easier matching
        std::string nameLow = name;
        std::transform(name.begin(), name.end(), nameLow.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        // TODO: Assign the proper portal texture and light material.
        if (nameLow.find("collision") != std::string::npos) {
            bounds.push_back({min, max});
        } else if ((pos = nameLow.find("volume")) != std::string::npos ||
                   (pos = nameLow.find("glass")) != std::string::npos) {
            // TODO: implement separate shaders, for now just ignore the mesh.
        } else if ((pos = nameLow.find("button")) != std::string::npos) {
            name = name.substr(0, pos);
            type = Mesh::Button;
        } else if ((pos = nameLow.find("light")) != std::string::npos) {
            name = name.substr(0, pos);
            type = Mesh::Light;
        } else if ((pos = nameLow.find("portal")) != std::string::npos) {
            name = name.substr(0, pos);
            type = Mesh::Portal;
        } else {
            // Process material textures
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            auto normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            auto roughnessMaps = loadMaterialTextures(material, aiTextureType_SHININESS);
            textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

            auto metalnessMaps = loadMaterialTextures(material, aiTextureType_REFLECTION);
            textures.insert(textures.end(), metalnessMaps.begin(), metalnessMaps.end());

            meshes.emplace_back(vertices, indices, textures, name, type);
        }
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type) {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::filesystem::path path(str.C_Str());

            // Check if texture was loaded before
            bool skip = false;
            for (unsigned int j = 0; j < loadedTextures.size(); j++) {
                if (loadedTextures[j].path == path) {
                    textures.push_back(loadedTextures[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip) {
                // Use the Texture class for loading
                Texture texture(path);
                texture.load();
                textures.push_back(texture);
                loadedTextures.push_back(texture);
            }
        }
        return textures;
    }
};
