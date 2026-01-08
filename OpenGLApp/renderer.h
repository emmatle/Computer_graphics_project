#pragma once

#include "utils.h"
#include "model.h"
#include "shader.h"
#include "texture.h"
#include "scene.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <glad/glad.h>

/**
 * @brief Manages OpenGL resources like shaders, buffers (VAO/VBO/FBO), and rendering logic.
 */
class Renderer : public IAssetManager {
    unsigned int pickingFBO = 0;
    unsigned int pickingDepthRBO = 0;
    unsigned int pickingTexture; // FBO Attachment
public:
    // Shader objects
    Shader renderingShader;
    Shader pickingShader;

    std::unordered_map<std::string, std::unique_ptr<Texture> > textures;
    std::unordered_map<std::string, std::unique_ptr<Model> > models;
    std::vector<DynamicTexture *> dynamicTextures;

    Renderer() : renderingShader("shaders/Blinn-Phong.glsl"),
                 pickingShader("shaders/Picking.glsl") {
    }

    Texture *getTexture(const std::string &path) override {
        std::string name;
        int width = 512;
        int height = 512;
        bool isDynamic = false;
        if (!path.empty() && name[0] == '#') {
            name = path.substr(1); // Skips the first character
            isDynamic = true;
            size_t pos1 = name.find('#');
            size_t pos2 = name.find_last_of('x');

            if (pos1 != std::string::npos && pos2 != std::string::npos &&
                pos2 > pos1 + 1 || pos2 != name.length() - 1) {
                width = std::stoi(name.substr(pos1 + 1, pos2 - pos1 - 1));
                height = std::stoi(name.substr(pos2 + 1));
                std::cout << width << " " << height << std::endl;
            }
            name = name.substr(0, pos1);
        } else {
            name = path;
        }
        auto [it, inserted] = textures.try_emplace(name, nullptr);

        if (inserted) {
            auto tex = isDynamic
                           ? std::make_unique<DynamicTexture>(name, width, height)
                           : std::make_unique<Texture>(name);
            if (!tex->load()) {
                textures.erase(it);
                return nullptr;
            }
            it->second = std::move(tex);
        }
        return it->second.get();
    }

    Model *getModel(const std::string &path) override {
        auto [it, inserted] = models.try_emplace(path, nullptr);

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

    bool loadModels(std::vector<std::shared_ptr<Object> > &objs);

    void free();

    static void clear(glm::vec4 color);

    void drawScene(const Scene &scene, bool picking = false) const;

    void updateTexture(const DynamicTexture &dyn, const Scene &scene);

    void onResize(int width, int height) const;

    int readObjFromCursor(const Scene &scene) const;
};

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
}

// TODO: free properly compiled shaders in case of error.
bool Renderer::compileShaders() {
    return renderingShader.compile() && pickingShader.compile();
}

bool Renderer::loadModels(std::vector<std::shared_ptr<Object> > &objs) {
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

    for (const auto &entry: textures) entry.second->free();
    for (const auto &entry: textures) entry.second->free();
}

inline void Renderer::clear(glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawScene(const Scene &scene, bool picking) const {
    Shader shader = picking ? pickingShader : renderingShader;

    glEnable(GL_DEPTH_TEST);

    shader.use();
    shader.set("view", scene.cam->getViewMatrix());
    shader.set("projection", scene.cam->getProjectionMatrix());

    if (!picking) {
        for (int i = 0; i < scene.lights.size(); i++) {
            scene.lights[i].apply(shader, i);
        }
        shader.setInt("numLights", static_cast<int>(scene.lights.size()));
        shader.set("viewPos", scene.cam->position);
    }

    for (const auto &obj: scene.objs) {
        if (!obj || !obj->model) continue;
        shader.set("model", obj->getModelMatrix());
        if (picking && obj->id) shader.setInt("id", obj->id);
        obj->model->draw(shader, !picking);
    }
}

void Renderer::updateTexture(const DynamicTexture &dyn, const Scene &scene) {
    glBindFramebuffer(GL_FRAMEBUFFER, dyn.sceneFBO);
    glViewport(0, 0, dyn.width, dyn.height);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawScene(scene);

    // Use default FBO and reset the viewport
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
}

int Renderer::readObjFromCursor(const Scene &scene) const {
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

    glEnable(GL_SCISSOR_TEST);
    // Scissor coordinates are (x, y, width, height) from bottom-left
    glScissor(fbWidth / 2, fbHeight / 2, 1, 1);

    // Draw only that 1x1 pixel area
    int clearValue = 0;
    glClearBufferiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawScene(scene, true);

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
