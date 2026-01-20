#include "renderer.h"
#include "scene.h"

// Asset manager implementations (unchanged logic, simplified)
Texture *Renderer::getTexture(const std::string &path) {
    std::string name = path;
    int width = 512, height = 512;
    bool isDynamic = false;
    if (!name.empty() && name[0] == '#') {
        name = path.substr(1);
        isDynamic = true;
        size_t pos1 = name.find('#');
        size_t pos2 = name.find_last_of('x');
        if (pos1 != std::string::npos && pos2 != std::string::npos &&
            pos2 > pos1 + 1 && pos2 != name.length() - 1) {
            width = std::stoi(name.substr(pos1 + 1, pos2 - pos1 - 1));
            height = std::stoi(name.substr(pos2 + 1));
        }
        name = name.substr(0, pos1);
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

Model *Renderer::getModel(const std::string &path) {
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

// GL buffer and shader helpers
bool Renderer::genBuffers() {
    // Picking FBO
    glGenFramebuffers(1, &pickingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

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

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: picking FBO is not complete" << std::endl;
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

bool Renderer::compileShaders() {
    if (!renderingShader.compile() || !pickingShader.compile() ||
        !textShader.compile() || !text3DShader.compile()) {
        return false;
    }
    return true;
}


bool Renderer::loadFont(const std::string &path, int size) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR: failed to initialize FreeType" << std::endl;
        return false;
    }

    std::string fontPath = getResourcePath(path);
    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
        std::cout << "ERROR: FreeType: failed to load font " << fontPath << std::endl;
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, size);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < NUM_CHARS; ++i) {
        if (FT_Load_Char(face, static_cast<unsigned long>(i), FT_LOAD_RENDER)) continue;
        unsigned int tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                     static_cast<GLsizei>(face->glyph->bitmap.width),
                     static_cast<GLsizei>(face->glyph->bitmap.rows),
                     0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        characters[i] = {
            tex,
            {face->glyph->bitmap.width, face->glyph->bitmap.rows},
            {face->glyph->bitmap_left, face->glyph->bitmap_top},
            static_cast<unsigned int>(face->glyph->advance.x)
        };
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // 2D VAO/VBO
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, QUAD_VERTS * VERTICES_2D_SIZE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, VERTICES_2D_SIZE, GL_FLOAT, GL_FALSE, VERTICES_2D_SIZE * sizeof(float), nullptr);

    // 3D VAO/VBO
    glGenVertexArrays(1, &textVAO3D);
    glGenBuffers(1, &textVBO3D);
    glBindVertexArray(textVAO3D);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO3D);
    glBufferData(GL_ARRAY_BUFFER, QUAD_VERTS * VERTICES_3D_SIZE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTICES_3D_SIZE * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERTICES_3D_SIZE * sizeof(float), (void *) (3 * sizeof(float)));

    glBindVertexArray(0);
    return true;
}

bool Renderer::loadModels(std::vector<std::shared_ptr<Object> > &objs) {
    for (auto &obj: objs) {
        if (obj->modelPath.empty()) continue;
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

    if (textVAO)
        glDeleteVertexArrays(1, &textVAO);
    if (textVBO)
        glDeleteBuffers(1, &textVBO);
    if (textVAO3D)
        glDeleteVertexArrays(1, &textVAO3D);
    if (textVBO3D)
        glDeleteBuffers(1, &textVBO3D);

    renderingShader.free();
    pickingShader.free();
    textShader.free();
    text3DShader.free();

    for (auto &ch: characters) {
        if (ch.textureID)
            glDeleteTextures(1, &ch.textureID);
    }

    for (auto &entry: textures) entry.second->free();
    for (auto &entry: models) entry.second->free();
}

void Renderer::clear(const glm::vec3 &color) {
    glClearColor(color.r, color.g, color.b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawScene(const Scene &scene, bool picking, int activeIndex) const {
    const Shader &shader = picking ? pickingShader : renderingShader;
    glEnable(GL_DEPTH_TEST);
    shader.use();
    shader.set("view", scene.cam->getViewMatrix());
    if (!picking) {
        clear(scene.clearColor);
        shader.set("projection", scene.cam->getProjectionMatrix());
        for (int i = 0; i < scene.lights.size(); ++i) scene.lights[i].apply(shader, i);
        shader.set("numLights", static_cast<int>(scene.lights.size()));
        shader.set("viewPos", scene.cam->position);
    } else {
        // TODO: Optimize by caching projection matrix
        glm::mat4 projectionMatrix = glm::perspective(scene.cam->fov, scene.cam->aspect, Camera::MIN_CLIPPING, 2.0f);
        shader.set("projection", projectionMatrix);
    }
    for (int i = 0; i < scene.objs.size(); ++i) {
        const auto &obj = scene.objs[i];
        if (activeIndex != -1 && i != activeIndex) continue;
        if (!obj || !obj->model) continue;
        shader.set("model", obj->getModelMatrix());
        if (picking) shader.set("id", obj->id);
        obj->model->draw(const_cast<Shader &>(shader), !picking);
    }
}

void Renderer::drawText(const std::string &text, float x, float y, float scale, const glm::vec3 &color, Align align) const {
    std::string clean = convertUTF8toLatin1(text);
    glm::mat4 projectionMatrix = glm::ortho(0.0f, static_cast<float>(fbWidth), static_cast<float>(fbHeight), 0.0f);

    float scaleFactor = fbScale * scale;
    // Compute total width for alignment
    float textWidth = 0.0f;
    for (unsigned char uc: clean) {
        const Character &ch = characters[uc];
        textWidth += static_cast<float>(ch.advance >> 6) * scaleFactor;
    }

    if (align == Align::Center) x -= textWidth * 0.5f;
    else if (align == Align::Right) x -= textWidth;

    textShader.use();
    textShader.set("textColor", color);
    textShader.set("projection", projectionMatrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (unsigned char uc: clean) {
        const Character &ch = characters[uc];
        float advance = static_cast<float>(ch.advance >> 6) * scaleFactor;
        if (ch.textureID == 0) {
            x += advance;
            continue;
        }

        float xpos = x + ch.bearing.x * scaleFactor;
        float ypos = y - ch.bearing.y * scaleFactor;
        float w = ch.size.x * scaleFactor;
        float h = ch.size.y * scaleFactor;

        float verts[QUAD_VERTS][VERTICES_2D_SIZE] = {
            {xpos, ypos + h, 0.0f, 1.0f},
            {xpos, ypos, 0.0f, 0.0f},
            {xpos + w, ypos, 1.0f, 0.0f},
            {xpos, ypos + h, 0.0f, 1.0f},
            {xpos + w, ypos, 1.0f, 0.0f},
            {xpos + w, ypos + h, 1.0f, 1.0f}
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTS);

        x += advance;
    }

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::drawText3D(const std::string &text, const glm::mat4 &model, const Camera &cam, float scale,
                          const glm::vec3 &color, Align align) const {
    std::string clean = convertUTF8toLatin1(text);
    scale *= 0.002f; // Adjust this scale factor as needed

    // Compute total width for alignment in 3D space (same as 2D but in object space units)
    float textWidth = 0.0f;
    for (unsigned char uc: clean) {
        const Character &ch = characters[uc];
        textWidth += static_cast<float>(ch.advance >> 6) * scale;
    }

    text3DShader.use();
    text3DShader.set("textColor", color);
    text3DShader.set("model", model);
    text3DShader.set("view", cam.getViewMatrix());
    text3DShader.set("projection", cam.getProjectionMatrix());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(textVAO3D);

    float penX = 0.0f;
    if (align == Align::Center) penX = -textWidth * 0.5f;
    else if (align == Align::Right) penX = -textWidth;

    for (unsigned char uc: clean) {
        const Character &ch = characters[uc];
        float advance = static_cast<float>(ch.advance >> 6) * scale;
        if (ch.textureID == 0) {
            penX += advance;
            continue;
        }

        float x0 = penX + ch.bearing.x * scale;
        float yTop = ch.bearing.y * scale;
        float yBot = (ch.bearing.y - ch.size.y) * scale;
        float w = ch.size.x * scale;

        float verts[QUAD_VERTS][VERTICES_3D_SIZE] = {
            {x0, yBot, 0.0f, 0.0f, 1.0f},
            {x0, yTop, 0.0f, 0.0f, 0.0f},
            {x0 + w, yTop, 0.0f, 1.0f, 0.0f},
            {x0, yBot, 0.0f, 0.0f, 1.0f},
            {x0 + w, yTop, 0.0f, 1.0f, 0.0f},
            {x0 + w, yBot, 0.0f, 1.0f, 1.0f}
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO3D);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, QUAD_VERTS);

        penX += advance;
    }
    glBindVertexArray(0);
}

void Renderer::updateTexture(const DynamicTexture &dyn, const Scene &scene) const {
    glBindFramebuffer(GL_FRAMEBUFFER, dyn.sceneFBO);
    glViewport(0, 0, dyn.width, dyn.height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawScene(scene);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbWidth, fbHeight);
}

int Renderer::readObjFromCursor(const Scene &scene) const {
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
    glEnable(GL_SCISSOR_TEST);
    glScissor(fbWidth / 2, fbHeight / 2, 1, 1);
    int clearValue = 0;
    glClearBufferiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawScene(scene, true);
    int pickedID = 0;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(fbWidth / 2, fbHeight / 2, 1, 1, GL_RED_INTEGER, GL_INT, &pickedID);
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

std::string Renderer::convertUTF8toLatin1(const std::string &in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c < 0x80) out.push_back(static_cast<char>(c));
        else if ((c & 0xE0) == 0xC0 && i + 1 < in.size()) {
            unsigned char next = static_cast<unsigned char>(in[++i]);
            unsigned char val = static_cast<unsigned char>(((c & 0x03) << 6) | (next & 0x3F));
            out.push_back(static_cast<char>(val));
        }
    }
    return out;
}
