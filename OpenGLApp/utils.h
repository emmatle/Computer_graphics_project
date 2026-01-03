#pragma once

#include <iostream>
#include <algorithm>
#include <filesystem>
#include "glm/glm.hpp"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION // macOS 10.14 - OpenGL API deprecated.
#endif

#ifndef RESOURCE_PATH // Already defined in the CMakeLists.txt, but kept as fallback
#ifdef ___APPLE__
#define RESOURCE_PATH "../Resources"
#else
#define RESOURCE_PATH "resources"
#endif
#endif

// --- Global variables ---

inline float deltaTime = 0.f; // Used for game synchronization
inline int fbWidth = 1920; // Framebuffer width
inline int fbHeight = 1080; // Framebuffer height
inline float aspect = 16.f / 9.f; // Aspect Ratio
inline bool debug = true; // Enables debug menu and flying camera

// Convert string to lowercase for case unsensitive comparison
inline std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
}

// Retrieves the correct path for game assets and check if file exists
inline std::string getResourcePath(const std::string &relative, bool silence = false) {
    auto file = std::filesystem::path(RESOURCE_PATH) / relative;
    file = file.lexically_normal().make_preferred();

    if (!silence && !std::filesystem::exists(file)) {
        std::cerr << "ERROR: file " << file << " not found" << std::endl;
        return "";
    }
    return file.string();
}

class Texture;
class Model;
class Object;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

// Interface class for managing game assets
class IAssetManager {
public:
    virtual ~IAssetManager() = default;

    virtual Texture *getTexture(const std::string &path) = 0;

    virtual Model *getModel(const std::string &path) = 0;
};
