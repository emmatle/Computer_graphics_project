#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "object.h"
#include "camera.h"

/**
 * @brief Manages game logic and handles objects and player state and input.
 */
class Game {
    static constexpr float SPRINT = 2.0f;
    static constexpr float INSPECT_ROT_SPEED = 80.f;
public:
    static constexpr glm::vec4 BG_COLOR{0.2, 0.3f, 0.4f, 1.f};
    static constexpr glm::vec4 MENU_COLOR{0.f, 0.f, 0.f, 1.f};

    enum Mode {
        Menu,
        Explore,
        Inspect,
    } mode = Explore;

    struct Input {
        bool w = false;
        bool s = false;
        bool a = false;
        bool d = false;
        bool shift = false;
    } input;

    std::vector<Object> objects;

    Camera player{{-0.2f, 1.6f, 2.1f}, {-17.0f, 0.0f, 0.0f}, 45.0f, true};
    Camera fixed{{0.0f, 0.0f, 3.0f}};
    Camera *current = nullptr;

    Object inspectedObj;
    Object *selectedObj = nullptr;

    float aspect;
    float playerSpeed = 2.5f;
    float mouseSensitivity = 0.05f;
    bool mouseDrag = false;
    bool debug = true;

    Game(float aspect = 16.f / 9.f, bool debug = false) : aspect(aspect), debug(debug) {
        player.aspectRatio = aspect;
        fixed.aspectRatio = aspect;
    }

    static Object objFromJson(nlohmann::json j, int &id, std::string &name, std::string &texture) {
        glm::vec3 pos{};
        glm::vec3 rot{};
        glm::vec3 scl{1.f};
        if (j.contains("id")) id = j["id"]; // Id is optional
        if (j.contains("name")) name = j["name"]; // Name is optional
        if (j.contains("position")) pos = {j["position"][0], j["position"][1], j["position"][2]};
        if (j.contains("rotation")) rot = {j["rotation"][0], j["rotation"][1], j["rotation"][2]};
        if (j.contains("scale")) scl = {j["scale"][0], j["scale"][1], j["scale"][2]};
        if (j.contains("texture")) texture = j["texture"]; // Texture is optional
        return {pos, rot, scl};
    }

    bool loadObjects() {
        std::filesystem::path file = getResource("objects.json");
        if (!exists(file)) return false;

        using json = nlohmann::json;
        std::ifstream in(file);

        in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        json j;

        try {
            in >> j;
        } catch (std::exception &e) {
            std::cerr << "ERROR: failed to read game data from " << file << ": " << e.what() << std::endl;
            return false;
        }

        if (!j.contains("objects")) return false;
        const json &s = j["objects"];

        for (const auto &entry: s) {
            int id = 0;
            std::string name, texture;
            Object obj = objFromJson(entry, id, name, texture);
            if (id != 0) obj.id = id;
            if (!name.empty()) obj.name = name;
            if (!texture.empty()) obj.texture = texture;
            objects.push_back(std::move(obj));
        }
        return true;
    }

    // TODO: add real collision detection.
    void checkCollision() {
        player.position.x = glm::clamp(player.position.x, -4.0f, 4.0f);
        player.position.z = glm::clamp(player.position.z, -4.0f, 4.0f);
        player.position.y = 1.6f;
    }

    void update(float deltaTime) {
        if (mode == Explore) {
            // Calculate the amount of the movement considering sprint (SHIFT)
            float velocity = playerSpeed * deltaTime;
            if (input.shift) velocity *= SPRINT;

            if (input.w) player.move(player.front, velocity, !debug);
            if (input.s) player.move(-player.front, velocity, !debug);
            if (input.a) player.move(-player.right, velocity, !debug);
            if (input.d) player.move(player.right, velocity, !debug);
        } else if (mode == Inspect) {
            // Object rotation in Inspect mode
            float amount = INSPECT_ROT_SPEED * deltaTime;

            if (input.w) inspectedObj.rotate(fixed.right, -amount, true);
            if (input.s) inspectedObj.rotate(fixed.right, amount, true);
            if (input.a) inspectedObj.rotate(fixed.up, -amount, true);
            if (input.d) inspectedObj.rotate(fixed.up, amount, true);
        }
    }

    // --- Callbacks ---

    // TODO: remove aspect from Camera.
    void onResize(float aspect) {
        player.aspectRatio = aspect;
        fixed.aspectRatio = aspect;
    }

    void onKey(int key, int action, int mods) {
        if (key == GLFW_KEY_TAB && action == GLFW_PRESS) debug = !debug; // toggle debug UI
        if (key == GLFW_KEY_E && action == GLFW_PRESS) Game::mode = Game::Explore;
    }

// TODO: update depending on gameplay mechanics.
    void onMouseButton(int button, int action, int objectId) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) mouseDrag = true;
            else if (action == GLFW_RELEASE) mouseDrag = false;
        }

        if (mode == Explore) {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                if (objectId == 0) return; // Backgound is selected

                for (auto &obj: Game::objects) {
                    if (objectId == obj.id) {
                        selectedObj = &obj;
                        inspectedObj = obj;

                        // Reset object rotation for inspection
                        inspectedObj.position = {};
                        inspectedObj.setRotation();

                        Game::mode = Game::Inspect;
                        break;
                    }
                }
            }
        } else if (mode == Inspect) {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) mode = Explore;
        }
    }

    void onMouseMovement(float xdelta, float ydelta) {
        if (mode == Explore) {
            player.yaw(mouseSensitivity * xdelta);
            player.pitch(mouseSensitivity * ydelta);
        } else if (mode == Inspect && mouseDrag) {
            inspectedObj.rotate(fixed.up, -3.f * mouseSensitivity * xdelta, true);
            inspectedObj.rotate(fixed.right, -3.f * mouseSensitivity * ydelta, true);
        }
    }

    // TODO: change zoom accordingly.
    void onMouseScroll(float yoffset) {
        player.zoom(yoffset);
        fixed.zoom(yoffset);
    }
};