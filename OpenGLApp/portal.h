#include "object.h"
#include <glad/glad.h>

class Portal {
public:
    int width;
    int height;
    float aspect;

    unsigned int sceneFBO = 0;
    unsigned int sceneDepthRBO = 0;
    unsigned int sceneColor;

    std::string name;

    Portal(std::string name, int width, int height) :
            name(std::move(name)),
            width(width),
            height(height),
            aspect(static_cast<float>(width) / static_cast<float>(height)) {}

    void gen() {
        // Create scene FBO
        glGenFramebuffers(1, &sceneFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

        // Create and Configure ID Texture (Color Attachment 0)
        glGenTextures(1, &sceneColor);
        glBindTexture(GL_TEXTURE_2D, sceneColor);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColor, 0);

        glGenRenderbuffers(1, &sceneDepthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, sceneDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRBO);

        // Check FBO status
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: scene FBO is not complete" << std::endl;
        }

        // Unbind FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void free() {
        if (sceneFBO) glDeleteBuffers(1, &sceneFBO);
        if (sceneDepthRBO) glDeleteRenderbuffers(1, &sceneDepthRBO);
        if (sceneColor) glDeleteTextures(1, &sceneColor);
    }
};