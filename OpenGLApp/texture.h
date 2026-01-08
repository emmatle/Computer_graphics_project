#pragma once

#include <iostream>
#include <glad/glad.h>
#include <stb_image.h>

/**
 * @brief Utility class for loading, managing, and binding OpenGL 2D textures.
 */
class Texture {
public:
    static constexpr int N_TYPES = 5;

    unsigned int id = 0;

    enum Type {
        Diffuse,
        Ambient,
        Normal,
        Roughness,
        Metalness
    } type;

    std::string path;

    Texture(const std::string &path = "") : type(determineType(path)), path(path) {
    }

    virtual ~Texture() = default;

    /**
     * @brief Loads image, generates texture object, and sets parameters.
     */
    virtual bool load() {
        static bool flipVertically = false;

        if (id != 0) return true; // Already loaded

        if (path.empty()) {
            std::cerr << "ERROR: texture path is empty" << std::endl;
            return false;
        }
        std::string fullPath = getResourcePath(path);

        // OpenGL expects the 0.0 coordinate on the Y-axis to be the bottom,
        // but images usually load with 0.0 at the top. This flips it to match.
        if (!flipVertically) {
            stbi_set_flip_vertically_on_load(true);
            flipVertically = true;
        }

        int width, height, nrChannels;
        unsigned char *data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = 0;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glGenTextures(1, &id);
            glBindTexture(GL_TEXTURE_2D, id);

            // Set texture wrapping: Repeat texture if coordinates > 1.0
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // Set texture filtering: Use Mipmaps for minification (far away), Linear for magnification (close up)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Upload the image data to the GPU and generate Mipmaps
            glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE,
                         data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Free the CPU-side image memory as it's now on the GPU
            stbi_image_free(data);
            return true;
        }

        std::cerr << "ERROR: failed to read texture " << path << std::endl;
        return false;
    }

    virtual void free() {
        if (id != 0) {
            glDeleteTextures(1, &id);
            id = 0;
        }
    }

    /**
     * @brief Activates the specific texture unit and binds this texture.
     */
    void use(unsigned int unit = 0) const {
        if (unit >= 16) {
            std::cerr << "WARNING: texture unit " << unit << " might be out of range" << std::endl;
        }
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

private:
    /**
     * @brief Helper to categorize texture based on common naming conventions.
     */
    Type determineType(const std::string &name) const {
        // Convert to lowercase for easier matching
        if (name.empty()) return Diffuse;

        std::string filename = stringToLower(name);

        if (filename.find("ambientocclusion") != std::string::npos ||
            filename.find("ambient") != std::string::npos ||
            filename.find("_ao") != std::string::npos) {
            return Ambient;
        }
        if (filename.find("normal") != std::string::npos ||
            filename.find("norm") != std::string::npos ||
            filename.find("bump") != std::string::npos) {
            return Normal;
        }
        if (filename.find("roughness") != std::string::npos ||
            filename.find("rough") != std::string::npos) {
            return Roughness;
        }
        if (filename.find("metalness") != std::string::npos ||
            filename.find("metal") != std::string::npos) {
            return Metalness;
        }
        return Diffuse; // Default fallback ("diffuse", "albedo", "color")
    }
};

class DynamicTexture : public Texture {
public:
    unsigned int sceneFBO = 0;
    unsigned int sceneDepthRBO = 0;
    const int width;
    const int height;
    const float aspect;

    DynamicTexture(const std::string &name, int width = 512, int height = 512)
        : width(width),
          height(height),
          aspect(static_cast<float>(width) / static_cast<float>(height)) {
        path = name;
        type = Diffuse;
    }

    bool load() override {
        // Create scene FBO
        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        // Create and Configure ID Texture (Color Attachment 0)
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);

        glGenRenderbuffers(1, &sceneDepthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

        // Check FBO status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: scene FBO is not complete" << std::endl;
            return false;
        }

        // Unbind FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void free() override {
        if (sceneFBO)
            glDeleteBuffers(1, &sceneFBO);
        if (sceneDepthRBO)
            glDeleteRenderbuffers(1, &sceneDepthRBO);
        if (id) {
            glDeleteTextures(1, &id);
            id = 0;
        }
    }
};