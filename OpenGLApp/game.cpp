#include "game.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

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
        auto obj = std::make_shared<Object>(entry);
        std::string sceneName = entry.value("scene", "room");
        if (sceneName == "room") {
            room.objs.push_back(obj.get());
            portal1.objs.push_back(obj.get());
        }

        // else if (sceneName == "inventory") {
        //     auto copy = *obj; // Copy
        //     inventory.objs.push_back(obj.get());
        // } else if (sceneName == "picture") {
        //     picture.objs.push_back(obj.get());
        // }

        // TODO: Link lights.
        // if (entry.contains("light")) {
        //     std::string name = entry["light"];
        //     lights[i].parent = obj.get();
        // }
        objects.push_back(obj);
    }
    return true;
}

void Game::update() {
    if (state == InGame) {
        // Calculate the amount of the movement considering sprint (SHIFT)
        float velocity = playerSpeed * deltaTime;
        if (input.shift) velocity *= SPRINT;

        if (input.w) player.move(player.front, velocity, !debug);
        if (input.s) player.move(-player.front, velocity, !debug);
        if (input.a) player.move(-player.right, velocity, !debug);
        if (input.d) player.move(player.right, velocity, !debug);

        static auto ref = getObject("Book");
        static std::vector brushes = {
            getObject("Brush1"),
            getObject("Brush2"),
            getObject("Brush3")
        };

        for (auto brush : brushes) {
            brush->rotate(Object::WRLD_UP, INSPECT_ANGULAR_SPEED * deltaTime);
        }

        if (!debug) {
            for (auto roomObjIt = room.objs.begin(); roomObjIt != room.objs.end();) {
                bool erased = false;
                for (auto brushIt = brushes.begin(); brushIt != brushes.end(); ++brushIt) {
                    if ((*brushIt)->name == (*roomObjIt)->name && (*brushIt)->checkCollision(player)) {
                        brushes.erase(brushIt);
                        roomObjIt = room.objs.erase(roomObjIt);
                        playerScore++; // TODO: Add sound effect.
                        erased = true;
                        break;
                    }
                }
                if (!erased) {
                    (*roomObjIt)->checkCollision(player, true);
                    ++roomObjIt;
                }
            }
        }
    } else if (state == Inventory) {
        // Object rotation in Inspect mode
        float amount = INSPECT_ANGULAR_SPEED * deltaTime;

        if (input.w) inspectedObj.rotate(fixed.right, -amount, true);
        if (input.s) inspectedObj.rotate(fixed.right, amount, true);
        if (input.a) inspectedObj.rotate(fixed.up, -amount, true);
        if (input.d) inspectedObj.rotate(fixed.up, amount, true);
    }
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
            selectedObj = obj;
            return obj.get();
        }
    }
    return nullptr;
}

void Game::inspectObject(int id) {
    if (id < 1) return;
    inspectedObj = *selectObject(id); // Copy.
    inspectedObj.setPosition();
    inspectedObj.setRotation();
    inspectedObj.setScale();
    state = Inventory;
}

// --- Callbacks ---

void Game::onKey() {
    if (state == InGame) {
        // if (input.esc) mode = Menu; // openMenu()
        // if (input.tab) mode = Inventory; // TODO: Add Inventory mode.
        // if (input.e && selectedObj) interact(selectedObj);
        if (input.q) playerScore++;
    } else if (state == Inventory) {
        if (input.esc) state = InGame;
    }
}

void Game::onMouseButton() {
    if (state == InGame) {
        if (input.rmb && selectedObj) {
            inspectedObj = *selectedObj; // Copy.
            inspectedObj.setPosition();
            inspectedObj.setRotation();
            inspectedObj.setScale();
            state = Inventory; // inspect(selectedObject)
        }
    }
}

void Game::onMouseMovement(float xdelta, float ydelta) {
    if (state == InGame) {
        player.yaw(glm::radians(mouseSensitivity * xdelta));
        player.pitch(glm::radians(mouseSensitivity * ydelta));
    } else if (state == Inventory && input.lmb) {
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
