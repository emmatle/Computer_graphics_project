#pragma once

#include "utils.h"
#include "model.h"
#include "shader.h"
#include "texture.h"
#include "object.h"
#include "portal.h"
#include "light.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <glad/glad.h>

/**
 * @brief Manages OpenGL resources like shaders, buffers (VAO/VBO/FBO), and rendering logic.
 */
class Renderer : public IAssetManager {
    static constexpr int N_LIGHTS = 1;

    unsigned int pickingFBO = 0;
    unsigned int pickingDepthRBO = 0;
    unsigned int pickingTexture; // FBO Attachment
public:
    // Shader objects
    Shader renderingShader;
    Shader pickingShader;

    std::unordered_map<std::string, std::unique_ptr<Texture> > textures;
    std::unordered_map<std::string, std::unique_ptr<Model> > models;

    Light lights[N_LIGHTS] = {
        {{-1.0f, 2.0f, -1.0f}}
    };

    Portal portals[2] = {
        {"Portal1", 682, 512},
        {"Portal2", 512, 682}
    };

    Renderer() : renderingShader("shaders/Blinn-Phong.glsl"),
                 pickingShader("shaders/Picking.glsl") {
    }

    Texture *getTexture(const std::string &path) override {
        std::string fullPath = getResourcePath(path);
        auto [it, inserted] = textures.try_emplace(fullPath, nullptr);

        if (inserted) {
            auto tex = std::make_unique<Texture>(path);
            if (!tex->load()) {
                textures.erase(it);
                return nullptr;
            }
            it->second = std::move(tex);
        }
        return it->second.get();
    }

    Model *getModel(const std::string &path) override {
        std::string fullPath = getResourcePath(path);
        auto [it, inserted] = models.try_emplace(fullPath, nullptr);

        if (inserted) {
            auto model = std::make_unique<Model>(this, path);
            if (!model->load()) {
                models.erase(it);
                return nullptr;
            }
            it->second = std::move(model);
        }
        return it->second.get();
    }

    void genBuffers();

    bool compileShaders();

    bool loadModels(std::vector<std::unique_ptr<Object> > &objs);

    void free();

    static void clear(glm::vec4 color);

    void drawScene(const std::vector<std::unique_ptr<Object> > &objs, const Camera &cam, bool picking = false) const;

    void drawObject(const Object &obj, const Camera &cam, bool picking = false) const;

    void updatePortal(const Portal &portal, const std::vector<std::unique_ptr<Object> > &objs, const Camera &cam);

    void onResize(int width, int height) const;

    int readObjFromCursor(const std::vector<std::unique_ptr<Object> > &objs, const Camera &cam) const;
};

// TODO: framebuffer size should change on window resize.
void Renderer::genBuffers() {
    // Create picking FBO
    glGenFramebuffers(1, &pickingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

    // Create and Configure ID Texture (Color Attachment 0)
    glGenTextures(1, &pickingTexture);
    glBindTexture(GL_TEXTURE_2D, pickingTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, fbWidth, fbHeight, 0, GL_RED_INTEGER, GL_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);

    glGenRenderbuffers(1, &pickingDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fbWidth, fbHeight);
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

bool Renderer::loadModels(std::vector<std::unique_ptr<Object> > &objs) {
    for (auto &obj: objs) {
        // Fetch the model asset from the manager using the path stored in the object
        if (obj->modelPath.empty()) continue; // Skip objects with unspecified models
        obj->model = getModel(obj->modelPath);
        obj->collisions = obj->model->collisions;
        if (!obj->model) {
            std::cerr << "ERROR: failed to load model for object: " << obj->name << std::endl;
            return false;
        }
    }
    return true;
}

void Renderer::free() {
    if (pickingFBO)
        glDeleteFramebuffers(1, &pickingFBO);
    if (pickingDepthRBO)
        glDeleteRenderbuffers(1, &pickingDepthRBO);
    if (pickingTexture)
        glDeleteTextures(1, &pickingTexture);

    renderingShader.free();
    pickingShader.free();

    for (auto &p: portals) p.free();

    for (const auto &entry: textures) entry.second->free();
    for (const auto &entry: textures) entry.second->free();
}

inline void Renderer::clear(glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawScene(const std::vector<std::unique_ptr<Object> > &objs, const Camera &cam, bool picking) const {
    Shader shader = picking ? pickingShader : renderingShader;

    glEnable(GL_DEPTH_TEST);

    shader.use();
    shader.set("view", cam.getViewMatrix());
    shader.set("projection", cam.getProjectionMatrix());

    if (!picking) {
        for (int i = 0; i < N_LIGHTS; i++) {
            lights[i].apply(shader, i);
        }
        shader.setInt("nLights", N_LIGHTS);
    }

    for (const auto &obj: objs) {
        shader.set("model", obj->getModelMatrix());
        if (picking && obj->model) {
            shader.setInt("id", obj->id);
            obj->model->draw(shader, false); // Skip textures setting when drawing IDs
        } else {
            shader.set("viewPos", cam.position);
            obj->model->draw(shader);
        }
    }
}

void Renderer::drawObject(const Object &obj, const Camera &cam, bool picking) const {
    Shader shader = picking ? pickingShader : renderingShader;

    glEnable(GL_DEPTH_TEST);

    shader.use();
    shader.set("view", cam.getViewMatrix());
    shader.set("projection", cam.getProjectionMatrix());

    if (!picking) {
        for (int i = 0; i < N_LIGHTS; i++) {
            lights[i].apply(shader, i);
        }
        shader.setInt("nLights", N_LIGHTS);
    }

    shader.set("model", obj.getModelMatrix());
    if (picking && obj.model) {
        shader.setInt("id", obj.id);
        obj.model->draw(shader, false); // Skip textures setting when drawing IDs
    } else {
        shader.set("viewPos", cam.position);
        obj.model->draw(shader);
    }
}

void Renderer::updatePortal(const Portal &portal, const std::vector<std::unique_ptr<Object> > &objs,
                            const Camera &cam) {
    glBindFramebuffer(GL_FRAMEBUFFER, portal.sceneFBO);
    glViewport(0, 0, portal.width, portal.height);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // TODO: Update remove clear color when using skyboxes.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    renderingShader.use();
    renderingShader.set("view", cam.getViewMatrix());
    renderingShader.set("projection", cam.getProjectionMatrix(portal.aspect));

    // TODO: Exclude portals for avoiding recursion.
    for (const auto &obj: objs) {
        renderingShader.set("model", obj->getModelMatrix());
        if (obj->model) obj->model->draw(renderingShader);
    }

    // Use default FBO and reset the viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
}

int Renderer::readObjFromCursor(const std::vector<std::unique_ptr<Object> > &objs, const Camera &cam) const {
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

    glEnable(GL_SCISSOR_TEST);
    // Scissor coordinates are (x, y, width, height) from bottom-left
    glScissor(fbWidth / 2, fbHeight / 2, 1, 1);

    // Draw only that 1x1 pixel area
    int clearValue = 0;
    glClearBufferiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawScene(objs, cam, true);

    // Read the pixel
    int pickedID = 0;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(fbWidth / 2, fbHeight / 2, 1, 1, GL_RED_INTEGER, GL_INT, &pickedID);

    // Cleanup: Disable Scissor so standard rendering isn't clipped!
    glDisable(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return pickedID;
}

void Renderer::onResize(int width, int height) const {
    glBindTexture(GL_TEXTURE_2D, pickingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, width, height, 0, GL_RED_INTEGER, GL_INT, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, pickingDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
}
