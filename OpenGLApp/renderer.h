#pragma once

#include <iostream>
#include <filesystem>

#include <glad/glad.h>

#include "shader.h"
#include "texture.h"
#include "portal.h"

/**
 * @brief Manages OpenGL resources like shaders, buffers (VAO/VBO/FBO), and rendering logic.
 */
class Renderer {
public:
    unsigned int pickingFBO = 0;
    unsigned int pickingDepthRBO = 0;

    // Shader objects
    Shader renderingShader;
    Shader pickingShader;

    // FBO attachments
    Texture pickingColor;

    Portal portals[2] = {{"Portal1", 682, 512},
                         {"Portal2", 512, 682}};

    Renderer() :
            renderingShader(getResource("shaders/rendering.glsl")),
            pickingShader(getResource("shaders/picking.glsl")) {}

    void genBuffers(int width, int height);

    bool compileShaders();

    bool loadModels(std::vector<Object> &objs);

    void free(std::vector<Object> &objects);

    static void clear(glm::vec4 color);

    void drawScene(const std::vector<Object> &objects, const Camera &cam, bool picking = false) const;

    void updatePortal(const Portal &portal, const std::vector<Object> &objects, const Camera &cam);

    int readObjectFromCursor(std::vector<Object> &objects, const Camera &cam, int width, int height) const;
};

// TODO: framebuffer size should change on window resize.
void Renderer::genBuffers(int width, int height) {
    // Create picking FBO
    glGenFramebuffers(1, &pickingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

    // Create and Configure ID Texture (Color Attachment 0)
    glGenTextures(1, &pickingColor.id);
    glBindTexture(GL_TEXTURE_2D, pickingColor.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingColor.id, 0);

    glGenRenderbuffers(1, &pickingDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, pickingDepthRBO);

    // Check FBO status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: picking FBO is not complete" << std::endl;
    }

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (auto &p: portals) p.gen();
}

// TODO: free properly compiled shaders in case of error.
bool Renderer::compileShaders() {
    return renderingShader.compile() && pickingShader.compile();
}

bool Renderer::loadModels(std::vector<Object> &objs) {
    for (auto &obj : objs) {
        if (!obj.load()) return false;
    }
    return true;
}

void Renderer::free(std::vector<Object> &objects) {
    if (pickingFBO) glDeleteFramebuffers(1, &pickingFBO);
    if (pickingDepthRBO) glDeleteRenderbuffers(1, &pickingDepthRBO);

    renderingShader.free();
    pickingShader.free();
    pickingColor.free();
    for (auto &p: portals) p.free();
    for (auto &obj : objects) obj.free();
}

void Renderer::clear(glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawScene(const std::vector<Object> &objects, const Camera &cam, bool picking) const {
    Shader shader = picking ? pickingShader : renderingShader;

    glEnable(GL_DEPTH_TEST);

    shader.use();
    shader.set("view", cam.getViewMatrix());
    shader.set("projection", cam.getProjectionMatrix(aspect));

    for (const auto &obj: objects) {
        if (picking) {
            shader.setInt("id", obj.id);
            obj.draw(shader, false); // Skip textures setting when drawing IDs
        } else {
            obj.draw(shader);
        }
    }
}

void Renderer::updatePortal(const Portal &portal, const std::vector<Object> &objects, const Camera &cam) {
    glBindFramebuffer(GL_FRAMEBUFFER, portal.sceneFBO);
    glViewport(0, 0, portal.width, portal.height);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // TODO: Update remove clear color when using skyboxes.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    renderingShader.use();
    renderingShader.set("view", cam.getViewMatrix());
    renderingShader.set("projection", cam.getProjectionMatrix(portal.aspect));

    // TODO: Exclude portals for avoiding recursion.
    for (const auto &obj: objects) {
        obj.draw(renderingShader);
    }

    // Use default FBO and reset the viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
}

/* TODO: code can be optimized by:
 *  - simplifying the algorithm when the cursor position is the center of the screen
 *  - reducing the picking framebuffer and viewport resolution
 *  - using a map instead of vector for storing objects
 *  - coloring with object pointers instead of integer values
 */
int Renderer::readObjectFromCursor(std::vector<Object> &objects, const Camera &cam, int width, int height) const {
    // 1. Bind Picking FBO
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
    glViewport(0, 0, width, height);

    // 2. Clear FBO
    int clearValue = 0;
    glClearBufferiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    drawScene(objects, cam, true);

    // --- Read pixel (center of screen) ---
    int pickedId = 0;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(width / 2, height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &pickedId);
    glReadBuffer(GL_BACK); // Restore default read buffer

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    return pickedId;
}
