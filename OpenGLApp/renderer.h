#pragma once

#include <iostream>
#include <filesystem>

#include <glad/glad.h>

#include "shader.h"
#include "texture.h"
#include "portal.h"
#include "cube.h" // TODO: remove when using real models.

/**
 * @brief Manages OpenGL resources like shaders, buffers (VAO/VBO/FBO), and rendering logic.
 */
class Renderer {
public:
    // Buffers
    unsigned int VBO = 0;
    unsigned int VAO = 0;

    unsigned int pickingFBO = 0;
    unsigned int pickingDepthRBO = 0;

    // Shader objects
    Shader renderingShader;
    Shader pickingShader;

    // FBO attachments
    Texture pickingColor;

    Texture undefinedTex;

    Portal portals[2] = {{"Portal_1", 682, 512}, {"Portal_2", 512, 682}};

    // Other textures used for game objects
    std::vector<Texture> textures;

    Renderer() :
            renderingShader(getResource("shaders/rendering.glsl")),
            pickingShader(getResource("shaders/picking.glsl")),
            undefinedTex(getResource("textures/uv_checker.png")) {}

    void genBuffers(int width, int height);

    bool compileShaders();

    bool loadTextures(std::vector<Object> &objs);

    void cleanup();

    static void clear(glm::vec4 color);

    void renderScene(const std::vector<Object> &objects, const Camera &cam);

    void updatePortal(const Portal & portal, const std::vector<Object> &objects, const Camera &cam);

    void drawObjects(const std::vector<Object> &objects, Shader &shader) const;

    void drawObject(const Object &obj, Shader &shader) const;

    int readObjectFromCursor(std::vector<Object> &objects, const Camera &cam, int width, int height) const;
};

// TODO: framebuffer size should change on window resize.
void Renderer::genBuffers(int width, int height) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX_DATA), VERTEX_DATA, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) nullptr);
    glEnableVertexAttribArray(0);

    // Texture coordinates attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *) offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(1);

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

    for (auto &p : portals) p.gen();
}

// TODO: free properly compiled shaders in case of error.
bool Renderer::compileShaders() {
    return renderingShader.compile() && pickingShader.compile();
}

bool Renderer::loadTextures(std::vector<Object> &objs) {
    std::filesystem::path texDir = getResource("textures");

    if (!exists(texDir)) {
        return false;
    }
    if (!std::filesystem::is_directory(texDir)) {
        std::cerr << "WARNING: " << texDir << "is not a directory" << std::endl;
        return false;
    }

    if (!undefinedTex.load()) return false;

    for (const auto &entry: std::filesystem::directory_iterator(texDir)) {
        if (std::filesystem::is_regular_file(entry)) {
            const auto &texPath = entry.path();
            Texture tex{texPath, texPath.filename().string()};

            // TODO: free properly loaded textures in case of error.
            if (!tex.load()) return false;

            // Assign the texture to the object
            for (auto &obj: objs) {
                for (const auto& p : portals) { // TODO: move portals to Game class.
                    if (obj.texture == p.name) {
                        obj.tex.id = p.sceneColor;
                        continue;
                    }
                }
                if (obj.texture == texPath.filename().string()) {
                    obj.tex = tex;
                }
            }
            textures.push_back(tex);
        }
    }
    return true;
}

void Renderer::cleanup() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (pickingFBO) glDeleteFramebuffers(1, &pickingFBO);
    if (pickingDepthRBO) glDeleteRenderbuffers(1, &pickingDepthRBO);

    renderingShader.free();
    pickingShader.free();
    pickingColor.free();
    undefinedTex.free();
    for (auto &p : portals) p.free();
    for (auto &tex: textures) tex.free();
}

void Renderer::clear(glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawObjects(const std::vector<Object> &objects, Shader &shader) const {
    static bool silenceWarning = false;

    glEnable(GL_DEPTH_TEST);

    glBindVertexArray(VAO);

    for (const auto& obj : objects) {
        shader.set("model", obj.getModelMatrix());

        shader.set("texture1", 0);
        if (obj.texture.empty()) {
            if (!silenceWarning) {
                std::cerr << "WARNING: undefined object texture(s) using UV checker as fallback" << std::endl;
                silenceWarning = true;
            }
            undefinedTex.use();
        } else obj.tex.use();

        // Draw a cube (36 vertices)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
}

void Renderer::drawObject(const Object &object, Shader &shader) const {
    drawObjects({object}, shader);
}

void Renderer::renderScene(const std::vector<Object> &objects, const Camera &cam) {
    renderingShader.use();
    renderingShader.set("view", cam.getViewMatrix());
    renderingShader.set("projection", cam.getProjectionMatrix(aspect));
    drawObjects(objects, renderingShader);
}

void Renderer::updatePortal(const Portal& portal, const std::vector<Object> &objects, const Camera &cam) {
    glBindFramebuffer(GL_FRAMEBUFFER, portal.sceneFBO);
    glViewport(0, 0, portal.width, portal.height);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    float portalAspect = static_cast<float>(portal.width) / static_cast<float>(portal.height);

    renderingShader.use();
    renderingShader.set("view", cam.getViewMatrix());
    renderingShader.set("projection", cam.getProjectionMatrix(portalAspect));

    for (const auto &obj: objects) {
        bool skip = false;
        // Excludes portals avoiding recursion
        for (const auto& p : portals) {
            if (obj.tex.id == p.sceneColor) skip = true;
        }
        if (!skip)  drawObject(obj, renderingShader);
    }

    // Use default FBO
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

    glEnable(GL_DEPTH_TEST);

    pickingShader.use();
    pickingShader.set("view", cam.getViewMatrix());
    pickingShader.set("projection", cam.getProjectionMatrix(aspect));

    glBindVertexArray(VAO);

    // 3. Draw Objects for Picking
    for (const auto &obj: objects) {
        if (obj.id == 0) continue; // Do not draw unselectable objects like ground

        // Use the dedicated picking shader and set object id
        pickingShader.use();
        pickingShader.set("id", obj.id);
        pickingShader.set("model", obj.getModelMatrix());

        // Draw a cube (
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);

    // --- Read pixel (center of screen) ---
    int pickedId = 0;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(width / 2, height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &pickedId);
    glReadBuffer(GL_BACK); // Restore default read buffer

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    return pickedId;
}
