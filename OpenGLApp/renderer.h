#pragma once

#include <iostream>
#include <filesystem>

#include <glad/glad.h>

#include "shader.h"
#include "texture.h"
#include "cube.h" // TODO: remove when using real models.

/**
 * @brief Manages OpenGL resources like shaders, buffers (VAO/VBO/FBO), and rendering logic.
 */
class Renderer {
public:
    // Buffers
    unsigned int VBO = 0;
    unsigned int VAO = 0;
    unsigned int FBO = 0;

    // Shader objects
    Shader renderingShader;
    Shader pickingShader;

    // FBO attachments
    Texture pickingTex;
    Texture depthTex;

    Texture undefinedTex;

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

    void drawObject(const Object &obj, const Camera &cam) const;

    int readObjectFromCursor(std::vector<Object> &objects, const Camera &cam, int width, int height) const;
};

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
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    // Create and Configure ID Texture (Color Attachment 0)
    glGenTextures(1, &pickingTex.id);
    glBindTexture(GL_TEXTURE_2D, pickingTex.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTex.id, 0);

    // Create and Configure Depth Texture Attachment
    glGenTextures(1, &depthTex.id);
    glBindTexture(GL_TEXTURE_2D, depthTex.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex.id, 0);

    // Specify that we only render to the color attachment 0
    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);

    // Check FBO status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: picking FBO is not complete" << std::endl;
    }

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
            Texture tex{texPath, texPath.filename()};

            // TODO: free properly loaded textures in case of error.
            if (!tex.load()) return false;
            // Assign the texture to the object
            for (auto &obj: objs) {
                if (obj.texture == texPath.filename()) {
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
    if (FBO) glDeleteFramebuffers(1, &FBO);

    renderingShader.free();
    pickingShader.free();
    depthTex.free();
    pickingTex.free();
    undefinedTex.free();
    for (auto &tex: textures) tex.free();
}

void Renderer::clear(glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawObject(const Object &obj, const Camera &cam) const {
    static bool silenceWarning = false;

    glEnable(GL_DEPTH_TEST);
    renderingShader.use();

    // Set matrices
    renderingShader.set("model", obj.getModelMatrix());
    // TODO: move view and projection matrices setting out for optimization.
    renderingShader.set("view", cam.getViewMatrix());
    renderingShader.set("projection", cam.getProjectionMatrix());

    renderingShader.set("texture1", 0);
    if (obj.texture.empty()) {
        if (!silenceWarning) {
            std::cerr << "WARNING: undefined object texture(s) using UV checker as fallback" << std::endl;
            silenceWarning = true;
        }
        undefinedTex.use();
    } else obj.tex.use();

    // Draw a cube (36 vertices)
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

/* TODO: code can be optimized by:
 *  - simplifying the algorithm when the cursor position is the center of the screen
 *  - reducing the picking framebuffer and viewport resolution
 *  - using a map instead of vector for storing objects
 *  - coloring with object pointers instead of integer values
 */
int Renderer::readObjectFromCursor(std::vector<Object> &objects, const Camera &cam, int width, int height) const {
    // 1. Bind Picking FBO
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glViewport(0, 0, width, height);

    // 2. Clear FBO
    int clearValue = 0;
    glClearBufferiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    // 3. Draw Objects for Picking
    for (const auto &obj: objects) {
        if (obj.id == 0) continue; // Do not draw unselectable objects like ground

        // Use the dedicated picking shader and set object id
        pickingShader.use();
        pickingShader.set("id", obj.id);
        pickingShader.set("model", obj.getModelMatrix());
        pickingShader.set("view", cam.getViewMatrix());
        pickingShader.set("projection", cam.getProjectionMatrix());

        // Draw a cube
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    // --- Read pixel (center of screen) ---
    int pickedId = 0;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(width / 2, height / 2, 1, 1, GL_RED_INTEGER, GL_INT, &pickedId);
    glReadBuffer(GL_BACK); // Restore default read buffer

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    return pickedId;
}
