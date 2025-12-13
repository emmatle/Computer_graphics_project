#pragma once

#include <iostream>
#include <filesystem>

#include <glad/glad.h>

#include <stb_image.h>

/**
 * @brief Utility class for loading, managing, and binding OpenGL 2D textures.
 */
class Texture {
public:
    unsigned int id = 0;
    std::filesystem::path path;
    std::string type; // e.g. "texture_diffuse", "texture_specular"

    Texture(std::filesystem::path path = "", std::string type = "")
            : path(std::move(path)), type(std::move(type)) {}

    /**
     * @brief Loads image, generates texture object, and sets parameters.
     */
    bool load() {
        if (id != 0) return true; // Already loaded

        if (!exists(path)) {
            std::cerr << "ERROR: failed to load texture " << path << std::endl;
            return false;
        }

        // OpenGL expects the 0.0 coordinate on the Y-axis to be the bottom,
        // but images usually load with 0.0 at the top. This flips it to match.
        stbi_set_flip_vertically_on_load(true);

        int width, height, nrChannels;
        unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &nrChannels, 0);

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
};