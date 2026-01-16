#pragma once

#include "utils.h"
#include "shader.h"
#include "texture.h"
#include "model.h"

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <vector>
#include <unordered_map>

class Object;
class Camera;
struct Scene;

// Text alignment options
enum class Align {
    Left,
    Center,
    Right
};

class Renderer : public IAssetManager {
public:
    // Asset manager
    Texture *getTexture(const std::string &path) override;

    Model *getModel(const std::string &path) override;

    bool genBuffers();

    bool compileShaders();

    bool loadFont(const std::string &fontPath, int size = 48);

    bool loadModels(std::vector<std::shared_ptr<Object> > &objs);

    void free();

    static void clear(const glm::vec3 &color);

    void drawScene(const Scene &scene, bool picking = false, int activeIndex = -1) const;

    void drawText(const std::string &text, float x, float y, float scale, const glm::vec3 &color = glm::vec3(1.0f),
                  Align align = Align::Left) const;

    void drawText3D(const std::string &text, const glm::vec3 &position, const Camera &cam, float scale = 1.f,
                    const glm::vec3 &color = glm::vec3(1.f), Align align = Align::Left) const;

    void updateTexture(const DynamicTexture &dyn, const Scene &scene) const;

    int readObjFromCursor(const Scene &scene) const;

    void onResize(int width, int height) const;

private:
    static constexpr int NUM_CHARS = 256;
    static constexpr int QUAD_VERTS = 6;
    static constexpr int VERTICES_2D_SIZE = 4;
    static constexpr int VERTICES_3D_SIZE = 5;

    // Picking FBO internals
    unsigned int pickingFBO = 0;
    unsigned int pickingDepthRBO = 0;
    unsigned int pickingTexture = 0;

    // Text VAO/VBO internals
    unsigned int textVAO = 0;
    unsigned int textVBO = 0;
    unsigned int textVAO3D = 0;
    unsigned int textVBO3D = 0;

    // Shaders
    Shader renderingShader{"shaders/Blinn-Phong.glsl"};
    Shader pickingShader{"shaders/Picking.glsl"};
    Shader textShader{"shaders/Text.glsl"};
    Shader text3DShader{"shaders/Text3D.glsl"};

    // Text renderer internals
    struct Character {
        unsigned int textureID = 0;
        glm::ivec2 size{0, 0};
        glm::ivec2 bearing{0, 0};
        unsigned int advance = 0;
    } characters[NUM_CHARS]{};

    // Asset storage
    std::unordered_map<std::string, std::unique_ptr<Texture> > textures;
    std::unordered_map<std::string, std::unique_ptr<Model> > models;
    std::vector<DynamicTexture *> dynamicTextures;

    // Helpers
    static std::string convertUTF8toLatin1(const std::string &in);
};
