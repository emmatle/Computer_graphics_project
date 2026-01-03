#pragma once

#include "game.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

    void Game::checkCollision() {
        for (auto &obj : objects) {
            obj->checkCollision(player);
        }
        // player.position.y = 1.75f; // Costrain the height
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
        } else if (mode == Inspect) {
            // Object rotation in Inspect mode
            float amount = INSPECT_ROT_SPEED * deltaTime;

            if (input.w) inspectedObj.rotate(fixed.right, -amount, true);
            if (input.s) inspectedObj.rotate(fixed.right, amount, true);
            if (input.a) inspectedObj.rotate(fixed.up, -amount, true);
            if (input.d) inspectedObj.rotate(fixed.up, amount, true);
        }

        if (!debug) checkCollision();
    }

    // --- Callbacks ---

    void Game::onKey(int key, int action, int mods) {
        if (key == GLFW_KEY_TAB && action == GLFW_PRESS) debug = !debug; // toggle debug UI
        if (key == GLFW_KEY_E && action == GLFW_PRESS) Game::mode = Game::Explore;
    }

    void Game::onMouseButton(int button, int action, int objectId) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) mouseDrag = true;
            else if (action == GLFW_RELEASE) mouseDrag = false;
        }

        if (mode == Explore) {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
                if (objectId == 0) return; // Backgound is selected

                for (auto &obj: objects) {
                    if (objectId == obj->id) {
                        selectedObj = obj.get();
                        inspectedObj = *obj;

                        // Reset object rotation for inspection
                        inspectedObj.position = {};
                        inspectedObj.setRotation();

                        mode = Inspect;
                        break;
                    }
                }
            }
        } else if (mode == Inspect) {
            if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) mode = Explore;
        }
    }

    void Game::onMouseMovement(float xdelta, float ydelta) {
        if (mode == Explore) {
            player.yaw(mouseSensitivity * xdelta);
            player.pitch(mouseSensitivity * ydelta);
        } else if (mode == Inspect && mouseDrag) {
            inspectedObj.rotate(fixed.up, -3.f * mouseSensitivity * xdelta, true);
            inspectedObj.rotate(fixed.right, -3.f * mouseSensitivity * ydelta, true);
        }
    }

    void Game::onMouseScroll(float yoffset) {
        player.zoom(yoffset);
        fixed.zoom(yoffset);
    }