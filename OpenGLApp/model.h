#pragma once

#include "utils.h"
#include "mesh.h"
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model {
    IAssetManager *assetManager;
    std::vector<Material> materials;

public:
    std::vector<Mesh> meshes;
    std::vector<AABB> collisions;
    std::string path;

    Model(IAssetManager *am, const std::string &p = "") : assetManager(am), path(getResourcePath(p)) {
    }

    bool load() {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR: ASSIMP: " << importer.GetErrorString() << std::endl;
            return false;
        }

        // Preload all materials
        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            Material mat;
            mat.name = scene->mMaterials[i]->GetName().C_Str();
            loadMaterialTextures(scene->mMaterials[i], mat);
            materials.push_back(mat);
        }

        // Process mesh hierarchy
        processNode(scene->mRootNode, scene);
        return true;
    }

    void free() { for (auto &mesh: meshes) mesh.free(); }

    void draw(Shader &shader, bool useTextures = true) const {
        for (const auto &mesh: meshes) mesh.draw(shader, useTextures);
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
        std::string nameLow = StringToLower(name);
        Mesh::Type type = Mesh::Default;

        if (nameLow.find("collision") != std::string::npos) {
            collisions.emplace_back(AABB{min, max});
            return;
        }

        // TODO: Handle the different cases.
        if (nameLow.find("button") != std::string::npos) type = Mesh::Button;
        else if (nameLow.find("light") != std::string::npos) type = Mesh::Light;
        else if (nameLow.find("portal") != std::string::npos) type = Mesh::Portal;

        // Assign correct preloaded material pointer
        Material *matPtr = (mesh->mMaterialIndex < materials.size()) ? &materials[mesh->mMaterialIndex] : nullptr;
        meshes.emplace_back(vertices, indices, matPtr, name, type);
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
