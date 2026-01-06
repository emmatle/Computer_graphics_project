#include "game.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

/**
 * @brief Manages game logic and handles objects and player state and input.
 */

bool Game::loadObjects() {
    std::filesystem::path file = getResourcePath("objects.json");
    if (file.empty()) return false;

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
    json &s = j["objects"];

    for (const auto &entry: s) {
        objects.emplace_back(std::make_unique<Object>(entry));
    }
    return true;
}

bool Game::checkCollision(bool pushOut) {
    bool collide = false;
    for (auto &obj: objects) {
        collide |= obj->checkCollision(player, pushOut);
        if (!pushOut && collide) return true;
    }
    return collide;
}

void Game::update() {
    if (mode == Explore) {
        // Calculate the amount of the movement considering sprint (SHIFT)
        float velocity = playerSpeed * deltaTime;
        if (input.shift) velocity *= SPRINT;

        if (input.w) player.move(player.front, velocity, !debug);
        if (input.s) player.move(-player.front, velocity, !debug);
        if (input.a) player.move(-player.right, velocity, !debug);
        if (input.d) player.move(player.right, velocity, !debug);

        static Object *ref = getObject("Book");
        if (ref) ref->rotate(Object::WRLD_UP, INSPECT_ANGULAR_SPEED * deltaTime);
    } else if (mode == Inspect) {
        // Object rotation in Inspect mode
        float amount = INSPECT_ANGULAR_SPEED * deltaTime;

        if (input.w) inspectedObj.rotate(fixed.right, -amount, true);
        if (input.s) inspectedObj.rotate(fixed.right, amount, true);
        if (input.a) inspectedObj.rotate(fixed.up, -amount, true);
        if (input.d) inspectedObj.rotate(fixed.up, amount, true);
    }

    if (!debug) checkCollision(true);
}

Object *Game::getObject(const std::string &name) {
    for (auto &obj: objects) {
        if (obj.get()->name == name) return obj.get();
    }
    return nullptr;
}

Object *Game::selectObject(int id) {
    if (id < 1) return nullptr;
    for (auto &obj: objects) {
        if (obj.get()->id == id) {
            selectedObj = obj.get();
            return obj.get();
        }
    }
    return nullptr;
}

// --- Callbacks ---

void Game::onKey() {
    if (mode == Explore) {
        // if (input.esc) mode = Menu; // openMenu()
        // if (input.tab) mode = Inventory; // TODO: Add Inventory mode.
        // if (input.e && selectedObj) interact(selectedObj);
        if (input.q) playerScore++;
    } else if (mode == Inspect) {
        if (input.esc) mode = Explore;
    }
}

void Game::onMouseButton() {
    if (mode == Explore) {
        if (input.rmb && selectedObj) {
            inspectedObj = *selectedObj; // TODO: Use shared pointers for multiple instances.
            inspectedObj.setPosition();
            inspectedObj.setRotation();
            mode = Inspect; // inspect(selectedObject)
        }
    }
}

void Game::onMouseMovement(float xdelta, float ydelta) {
    if (mode == Explore) {
        player.yaw(glm::radians(mouseSensitivity * xdelta));
        player.pitch(glm::radians(mouseSensitivity * ydelta));
    } else if (mode == Inspect && input.lmb) {
        inspectedObj.rotate(fixed.up, -glm::radians(mouseSensitivity * xdelta), true);
        inspectedObj.rotate(fixed.right, -glm::radians(mouseSensitivity * ydelta), true);
    }
}

void Game::onMouseScroll(float yoffset) {
    float amount = glm::radians(3.f * yoffset);
    player.zoom(amount);
    fixed.zoom(amount);
}

void Game::onResize(int width, int height) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    player.setAspect(aspect);
    fixed.setAspect(aspect);
}