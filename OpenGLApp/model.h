#pragma once

#include "mesh.h"
#include "utils.h"

#include <vector>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model {
    IAssetManager *assetManager;

public:
    std::vector<Mesh> meshes;
    std::vector<AABB> collisions;
    std::string path;

    Model(IAssetManager *am, const std::string &path = "")
        : assetManager(am), path(getResourcePath(path)) {
    }

    // Loads a model using ASSIMP
    bool load() {
        if (path.empty()) {
            std::cerr << "ERROR: unspecified model path" << std::endl;
            return false;
        }

        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                       aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR: ASSIMP: " << importer.GetErrorString() << std::endl;
            return false;
        }

        processNode(scene->mRootNode, scene);
        return true;
    }

    // Free GPU resources
    void free() {
        for (auto &mesh: meshes) mesh.free();
    }

    // Draws the model by iterating through all its meshes
    void draw(Shader &shader, bool useTextures = true) const {
        for (auto &mesh: meshes) mesh.draw(shader, useTextures);
    }

private:
    // Recursively processes nodes
    void processNode(aiNode *node, const aiScene *scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) processNode(node->mChildren[i], scene);
    }

    void processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Used for bounding boxes
        glm::vec3 min(std::numeric_limits<float>::max());
        glm::vec3 max(-std::numeric_limits<float>::max());

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex = {};
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
            } else {
                // Fallback values
                vertex.texCoords = glm::vec2(0.0f, 0.0f);
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
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
        std::string name = mesh->mName.C_Str();
        std::string nameLow = toLower(name);

        // TODO: Assign the proper portal texture and light material.
        if (nameLow.find("collision") != std::string::npos) {
            collisions.emplace_back(AABB{min, max});
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

            Mesh newMesh(vertices, indices, name, type);
            loadMaterialTextures(material, newMesh);
            meshes.emplace_back(std::move(newMesh));
        }
    }

    void loadMaterialTextures(aiMaterial *mat, Mesh &mesh) {
        aiTextureType types[] = {
            aiTextureType_DIFFUSE,
            aiTextureType_SPECULAR,
            aiTextureType_AMBIENT,
            aiTextureType_HEIGHT, // Often used for Normals
            aiTextureType_NORMALS,
            aiTextureType_SHININESS, // Roughness
            aiTextureType_REFLECTION // Metalness
        };

        for (const auto type: types) {
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
                aiString name;
                mat->GetTexture(type, i, &name);

                Texture *tex = assetManager->getTexture(name.C_Str());

                if (tex) mesh.textures[tex->type] = tex;
            }
        }
    }
};
