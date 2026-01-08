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
            mirror.objs.push_back(obj.get());
        } // else if (sceneName == "inventory") {
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
        if (entry.contains("type")) {
            std::string name = entry["type"];
            if (name == "Mirror") {
                mirrorView.parent = obj.get();
            } else if (name == "Picture") {
                pictureView.parent = obj.get();
            }
        }
        objects.push_back(obj);
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
    if (state == InGame) {
        // Calculate the amount of the movement considering sprint (SHIFT)
        float velocity = playerSpeed * deltaTime;
        if (input.shift) velocity *= SPRINT;

        if (input.w) player.move(player.front, velocity, !debug);
        if (input.s) player.move(-player.front, velocity, !debug);
        if (input.a) player.move(-player.right, velocity, !debug);
        if (input.d) player.move(player.right, velocity, !debug);

        updateMirror(mirrorView);

        static auto ref = getObject("Book");
        if (ref) ref->rotate(Object::WRLD_UP, INSPECT_ANGULAR_SPEED * deltaTime);

        if (!debug) checkCollision(true);
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

void Game::updateMirror(Object &mirror) {
    // Convert to the mirror's local space
    glm::mat4 mirrorInv = mirror.getInverseModelMatrix();
    glm::vec3 playerLocalPos = glm::vec3(mirrorInv * glm::vec4(player.position, 1.0f));

    glm::vec3 reflectedLocalPos = playerLocalPos;
    reflectedLocalPos.z = -playerLocalPos.z; // Flip depth
    reflectedLocalPos.x = -playerLocalPos.x; // Flip horizontal (lateral inversion)

    // Since parent is mirror object we just set the local coordinates
    mirrorView.setPosition(reflectedLocalPos);

    // The mirror camera should look at the "reflected" target.
    // We can reflect the player's local front vector similarly.
    glm::vec3 playerLocalFront = glm::normalize(glm::vec3(mirrorInv * glm::vec4(player.front, 0.0f)));
    glm::vec3 reflectedLocalFront = playerLocalFront;
    reflectedLocalFront.z = -playerLocalFront.z;
    reflectedLocalFront.x = -playerLocalFront.x;

    // Set rotation based on the reflected look direction
    mirrorView.setRotation(reflectedLocalFront, true);
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
