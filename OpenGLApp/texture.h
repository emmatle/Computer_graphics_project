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

    Texture(const std::string &path = "") : path(getResourcePath(path)) {
        if (!path.empty()) assignType();
    }

    /**
     * @brief Loads image, generates texture object, and sets parameters.
     */
    bool load() {
        static bool flipVertically = false;
        if (path.empty()) {
            std::cerr << "ERROR: unspecified texture path" << std::endl;
            return false;
        }

        if (id != 0) return true; // Already loaded

        // OpenGL expects the 0.0 coordinate on the Y-axis to be the bottom,
        // but images usually load with 0.0 at the top. This flips it to match.
        if (!flipVertically) {
            stbi_set_flip_vertically_on_load(true);
            flipVertically = true;
        }

        int width, height, nrChannels;
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

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
        } else {
            std::cerr << "ERROR: failed to read texture file: " << path << std::endl;
            return false;
        }
    }

    void free() {
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
    void assignType() {
        // Convert to lowercase for easier matching
        std::string filename = toLower(path);

        type = Diffuse; // Default fallback ("diffuse", "albedo", "color")

        if (filename.find("ambientocclusion") != std::string::npos ||
            filename.find("ambient") != std::string::npos ||
            filename.find("_ao") != std::string::npos) {
            type = Ambient;
        } else if (filename.find("normal") != std::string::npos ||
                   filename.find("norm") != std::string::npos ||
                   filename.find("bump") != std::string::npos) {
            type = Normal;
        } else if (filename.find("roughness") != std::string::npos ||
                   filename.find("rough") != std::string::npos) {
            type = Roughness;
        } else if (filename.find("metalness") != std::string::npos ||
                   filename.find("metal") != std::string::npos) {
            type = Metalness;
        }
    }
};
