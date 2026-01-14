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
        std::shared_ptr<Object> obj = std::make_shared<Object>(entry);

        if (entry.contains("player")) {
            player.setPosition(entry["player"].value("position", glm::vec3(0.f)));
            player.setRotation(glm::radians(entry["player"].value("rotation", glm::vec3(0.f))));
        } else if (entry.contains("portal")) {
            int index = entry["portal"].value("index", -1);
            if (index == -1) continue;
            // TODO: Implement portals properly.
        }

        if (entry.contains("animation")) {
            obj->animation = entry["animation"].get<Animation>();
        }

        std::string sceneName = entry.value("scene", "room");
        if (sceneName == "room") {
            room.objs.push_back(obj.get());
            portals[0].objs.push_back(obj.get());
        } else if (sceneName == "portal") {
            portals[1].objs.push_back(obj.get());
        }
        objects.push_back(obj);
    }

    return true;
}

void Game::update() {
    if (playerScore >= 3) state = Credits;
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

        for (auto &obj: objects) {
            obj->onUpdate();
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
    } else if (state == Inspection) {
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

// TODO: Optimize with hashmap.
Object *Game::findObject(int id) {
    if (id < 1) return nullptr;
    std::cout << "Searching for object withv" << id << std::endl;
    for (auto &obj: objects) {
        if (obj.get()->id == id) {
            selectedObj = obj;
            return obj.get();
        }
    }
    return nullptr;
}

void Game::viewCanvas(int index) {}

void Game::useObject(Object *obj) {
    size_t pos = obj->name.find("Button");
    if (pos != std::string::npos) {
        pos += 6;
        std::string num = obj->name.substr(pos);
        if (!num.empty()) {
            if (std::isdigit(num[0])) {
                if (enteredCode.length() >= password.length()) {
                    enteredCode.clear();
                } else {
                    enteredCode.append(num);
                }
            }
            if (num == "Clear") {
                if (!enteredCode.empty()) {
                    enteredCode.pop_back();
                }
            }
            if (num == "Enter") {
                if (enteredCode == password) {
                    std::cout << "Correct code entered! Door unlocked." << std::endl;
                    if (auto door = getObject("Safe Door")) door->animation.play();
                } else {
                    // Incorrect code, reset entered code
                    std::cout << "Incorrect code. Try again." << std::endl;
                    enteredCode.clear();
                }
            }
        }
        return;
    }
    pos = obj->name.find("Portal");
    if (pos != std::string::npos) {
        pos += 6;
        std::string num = obj->name.substr(pos);
        if (!num.empty() && !std::isdigit(num[0])) {
            // TODO: Transition player camera to canvas view.
            int index = std::stoi(num) - 1;
            if (!canvas[index].isSolved) viewCanvas(index);
            return;
        }
    }
    if (obj->name == "Safe Door") return; // Do not open the door

    // Try to open object
    if (!obj->animation.empty()) {
        obj->animation.toggle();
        return;
    }

    // Inspect object
    inspectedObj = *obj; // Copy.
    inspectedObj.setPosition();
    inspectedObj.setRotation();
    inspectedObj.setScale();
    state = Inspection;
}

// --- Callbacks ---

void Game::onKey() {
    if (state == InGame) {
        // if (input.esc) mode = Menu; // openMenu()
        // if (input.tab) mode = Inventory; // TODO: Add Inventory mode.
        // if (input.e && selectedObj) interact(selectedObj);
        if (input.q) playerScore++;
    } else if (state == Inspection) {
        if (input.esc) state = InGame;
    }
}

void Game::onMouseButton() {
    if (state == InGame) {
        if (input.lmb && hoveredObj) {
            useObject(hoveredObj);
        }
    }
}

void Game::onMouseMovement(float xdelta, float ydelta) {
    if (state == InGame) {
        player.yaw(glm::radians(mouseSensitivity * xdelta));
        player.pitch(glm::radians(mouseSensitivity * ydelta));
    } else if (state == Inspection && input.lmb) {
        inspectedObj.rotate(fixed.up, -glm::radians(mouseSensitivity * xdelta), true);
        inspectedObj.rotate(fixed.right, -glm::radians(mouseSensitivity * ydelta), true);
    }
}

void Game::onMouseScroll(float yoffset) {
    if (state == InGame || state == Inspection) {
        float amount = glm::radians(3.f * yoffset);
        player.zoom(amount);
        fixed.zoom(amount);
    }
}

void Game::onResize(int width, int height) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    player.setAspect(aspect);
    fixed.setAspect(aspect);
}
