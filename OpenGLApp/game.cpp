#include "game.h"
#include "renderer.h"
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
        }

        if (entry.contains("animation")) {
            obj->animation = entry["animation"].get<Animation>();
        }

        std::string sceneName = entry.value("scene", "room");
        if (sceneName == "room") {
            if (entry.value("name", "").find("Brush") == std::string::npos) {
                room.objs.push_back(obj.get());
            }
        } else if (sceneName == "canvas") {
            canvas.objs.push_back(obj.get());
        }
        objects.push_back(obj);
    }

    return true;
}

void Game::update() {
    time += deltaTime;

    if (state == InGame) {
        // Calculate the amount of the movement considering sprint (SHIFT)
        float velocity = playerSpeed * deltaTime;
        if (input.shift) velocity *= SPRINT;

        if (input.w) player.move(player.front, velocity, !debug);
        if (input.s) player.move(-player.front, velocity, !debug);
        if (input.a) player.move(-player.right, velocity, !debug);
        if (input.d) player.move(player.right, velocity, !debug);

        for (auto &obj: objects) {
            obj->onUpdate();
        }

        if (equippedObj) {
            equippedObj->setPosition(player.position + player.front * 0.5f + player.right * 0.3f - player.up * 0.2f);
            equippedObj->setRotation(player.orientation);
        }

        if (collectionTimer > 0.f) {
            collectionTimer -= deltaTime;
            if (collectionTimer <= 0.f) {
                collectionTimer = 0.f;
                // Clear brushes from room if they weren't collected
                for (auto it = room.objs.begin(); it != room.objs.end(); ) {
                    if ((*it)->name.find("Brush") != std::string::npos) {
                        it = room.objs.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }

        if (!debug) {
            for (auto roomObjIt = room.objs.begin(); roomObjIt != room.objs.end();) {
                Object* obj = *roomObjIt;
                bool erased = false;

                if (collectionTimer > 0 && obj->name.find("Brush") != std::string::npos) {
                    if (obj->checkCollision(player)) {
                        roomObjIt = room.objs.erase(roomObjIt);
                        collectedItems++; // TODO: Add sound effect.
                        erased = true;
                        
                        if (collectedItems >= 4) {
                            collectionTimer = 0.f; // Success
                        }
                    }
                }

                if (!erased) {
                    obj->checkCollision(player, true);
                    ++roomObjIt;
                }
            }
        }
    } else if (state == Inventory) {
        float amount = glm::radians(90.f) * deltaTime;

        if (inventory.objs.empty()) return;

        if (input.w) inventory.objs[inventoryIndex]->rotate(Object::WRLD_RIGHT, -amount, true);
        if (input.s) inventory.objs[inventoryIndex]->rotate(Object::WRLD_RIGHT, amount, true);
        if (input.a) inventory.objs[inventoryIndex]->rotate(Object::WRLD_UP, -amount, true);
        if (input.d) inventory.objs[inventoryIndex]->rotate(Object::WRLD_UP, amount, true);
    } else if (state == Canvas) {
        // Portal view rotation in Puzzle mode
        float amount = glm::radians(45.f) * deltaTime;
        if (input.shift) amount *= 0.2f;

        for (const auto &obj: canvas.objs) {
            if (input.w) obj->rotate(Object::WRLD_RIGHT, -amount, true);
            if (input.s) obj->rotate(Object::WRLD_RIGHT, amount, true);
            if (input.a) obj->rotate(Object::WRLD_UP, -amount, true);
            if (input.d) obj->rotate(Object::WRLD_UP, amount, true);
        }
    }
}

void Game::draw(int fbWidth, int fbHeight, float fbScale) {
    if (!renderer) return;

    static auto canvasTex = dynamic_cast<DynamicTexture *>(renderer->getTexture("#Canvas"));
    if (canvasTex) renderer->updateTexture(*canvasTex, canvas);

    // Scene rendering
    if (state == InGame || state == Canvas) {
        int id = renderer->readObjFromCursor(room);
        hoveredObj = findObject(id); // TODO: Display text hint if present.

        if (state == Canvas && canvasTex) renderer->updateTexture(*canvasTex, canvas);
        renderer->drawScene(room);

        // Crosshair
        renderer->drawText(
            ".",
            static_cast<float>(fbWidth) / 2.0f,
            static_cast<float>(fbHeight) / 2.0f,
            fbScale,
            glm::vec3(0.8f, 0.8f, 0.8f),
            Align::Center
        );

        if (collectionTimer > 0.f) {
            renderer->drawText(
                "Collect all the brushes (" + std::to_string(collectedItems) + "/4)",
                25.0f,
                60.0f,
                fbScale,
                glm::vec3(1.0f, 1.0f, 0.0f)
            );
            renderer->drawText(
                "Time left: " + std::to_string(static_cast<int>(collectionTimer)) + "s",
                25.0f,
                120.0f,
                fbScale,
                glm::vec3(1.0f, 0.0f, 0.0f)
            );
        }
    } else if (state == Inventory) {
        renderer->drawScene(inventory, false, inventoryIndex);
    } else if (state == Credits) {
        Renderer::clear();
        renderer->drawText(
            "Yippee, you are a star!",
            static_cast<float>(fbWidth) / 2.0f,
            static_cast<float>(fbHeight) / 2.0f,
            fbScale,
            glm::vec3(1.0f, 1.0f, 0.0f),
            Align::Center
        );
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
    for (auto &obj: objects) {
        if (obj.get()->id == id) {
            selectedObj = obj;
            return obj.get();
        }
    }
    return nullptr;
}

void Game::interact(Object *obj) {
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
    pos = obj->name.find("Palette");
    if (pos != std::string::npos) {
        if (collectedItems > 4) return;

        collectionTimer = 10.f;
        collectedItems = 0;

        std::vector<std::string> brushNames = {"Brush0", "Brush1", "Brush2", "Brush3"};
        for (const auto& name : brushNames) {
            Object* brush = getObject(name);
            if (brush) {
                // Check if it's already in the room
                bool inRoom = false;
                for (auto roomObj : room.objs) {
                    if (roomObj == brush) {
                        inRoom = true;
                        break;
                    }
                }
                if (!inRoom) {
                    brush->animation.play();
                    room.objs.push_back(brush);
                }
            }
        }
        return;
    }

    pos = obj->name.find("Easle");
    if (pos != std::string::npos) {
        lastPos = player.position;
        lastRot = player.rotation;
        player.setPosition(obj->position - obj->front * 2.0f + obj->up * 2.12f);
        player.setRotation(obj->rotation - glm::vec3(glm::radians(20.f), 0.0f, 0.f));
        state = Canvas;
        return;
    }

    if (obj->name == "Safe Door") return; // Do not open the door

    if (obj->name == "Door") {
        if (doorUnlocked) {
            obj->animation.play();
            return;
        }
        if (equippedObj && equippedObj->name.find("Key") != std::string::npos) {
            doorUnlocked = true;
            obj->animation.play();
            std::cout << "Door unlocked with " << equippedObj->name << "!" << std::endl;
        } else {
            std::cout << "The door is locked. You need a key." << std::endl;
        }
        return;
    }

    // Used mostly for opening/closing objects
    if (!obj->animation.empty()) {
        obj->animation.toggle();
        return;
    }
    
    for (auto it = room.objs.begin(); it != room.objs.end(); ++it) {
        if (*it == obj) {
            inventory.objs.push_back(obj);
            room.objs.erase(it);
            break;
        }
    }

    obj->setPosition();
    obj->setRotation();
    inventoryIndex = static_cast<int>(inventory.objs.size()) - 1;
    state = Inventory;
}

// --- Callbacks ---

void Game::onKey() {
    if (state == InGame) {
        if (input.tab) state = Inventory;

        if (input.q && !inventory.objs.empty()) {
            inventoryIndex = (inventoryIndex - 1 + (int) inventory.objs.size()) % (int) inventory.objs.size();
        }
        if (input.e && !inventory.objs.empty()) {
            inventoryIndex = (inventoryIndex + 1) % (int) inventory.objs.size();
        }
    } else if (state == Inventory) {
        if (input.esc) state = InGame;
    } else if (state == Canvas) {
        if (input.esc) {
            state = InGame;
            player.setPosition(lastPos);
            player.setRotation(lastRot);
        }
    }
}

void Game::onMouseButton() {
    if (state == InGame) {
        if (input.rmb) {
            if (equippedObj) {
                // Unequip
                std::cout << "Unequipped " << equippedObj->name << std::endl;
                equippedObj->setPosition();
                equippedObj->setRotation();
                inventory.objs.push_back(equippedObj);

                // Remove from room
                for (auto it = room.objs.begin(); it != room.objs.end(); ++it) {
                    if (*it == equippedObj) {
                        room.objs.erase(it);
                        break;
                    }
                }

                equippedObj = nullptr;
                inventoryIndex = (int) inventory.objs.size() - 1;
            } else if (!inventory.objs.empty()) {
                // Equip
                equippedObj = inventory.objs[inventoryIndex];
                room.objs.push_back(equippedObj);
                inventory.objs.erase(inventory.objs.begin() + inventoryIndex);
                if (inventoryIndex >= inventory.objs.size()) inventoryIndex = (int) inventory.objs.size() - 1;
                if (inventoryIndex < 0) inventoryIndex = 0;
                std::cout << "Equipped " << equippedObj->name << std::endl;
            }
        } else if (input.lmb) {
            if (hoveredObj) {
                interact(hoveredObj);
            }
        }
    }
}

void Game::onMouseMovement(float xdelta, float ydelta) {
    if (state == InGame) {
        player.yaw(glm::radians(mouseSensitivity * xdelta));
        player.pitch(glm::radians(mouseSensitivity * ydelta));
    } else if (state == Inventory && input.lmb) {
        inventory.objs[inventoryIndex]->rotate(inventoryView.up, -glm::radians(mouseSensitivity * xdelta), true);
        inventory.objs[inventoryIndex]->rotate(inventoryView.right, -glm::radians(mouseSensitivity * ydelta), true);
    } else if (state == Canvas) {
        player.yaw(0.1f * glm::radians(mouseSensitivity * xdelta));
        player.pitch(0.1f * glm::radians(mouseSensitivity * ydelta));

        canvasView.yaw(0.05f * glm::radians(mouseSensitivity * xdelta));
        canvasView.pitch(0.05f * glm::radians(mouseSensitivity * ydelta));
    }
}

void Game::onMouseScroll(float yoffset) {
    if (state == InGame || state == Inventory) {
        float amount = glm::radians(3.f * yoffset);
        player.zoom(amount);
    }
}

void Game::onResize(int width, int height) {
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    player.setAspect(aspect);
    inventoryView.setAspect(aspect);
}
