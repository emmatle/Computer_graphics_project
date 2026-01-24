#include "game.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
#include <vector>

#include "application.h"
#include "audio_manager.h"
#include "renderer.h"
#include "utils.h"

bool Game::loadObjects() {
  AudioManager::instance().init();

  glm::vec3 placeholderPositions[] = {
    {-0.82238, 0.586187, -1.21612},
    {-0.82238, 0.586187, -1.25784},
    {-0.82238, 0.586187, -1.29852},
    {-0.82238, 0.586187, -1.34151},
    {-0.82238, 0.586187, -1.38418},
    {-0.82238, 0.586187, -1.42456},
    {-0.82238, 0.586187, -1.46557}
  };

  glm::vec3 pencilPositions[14] = {
    {-0.769108, 0.427767, -1.27026},
    {-1.03855, 0.673286, -0.761964},
    {-0.738341, 1.81706, -1.39378},
    {1.6047, 0.822633, -2.02193},
    {3.95431, 0.712525, -0.941276},
    {6.14804, 0.519597, 0.528832},
    {4.56654, 1.36895, 1.22153},
    {6.66232, 0.36859, -1.29101},
    {6.74951, 1.50641, 1.18258},
    {6.74073, 0.547545, 1.35594},
    {1.60812, 0.323185, -2.41071},
    {-0.497238, 0.427767, -2.60119},
    {4.38636, 0.007796, -0.685579},
    {3.74331, 0.763184, 1.6281}
  };

  glm::vec3 pencilRotations[14] = {
    { 90, 60, 0 },
    { 90, 180, 0 },
    { 90, 0, 0 },
    { 0, 0, 0 },
    { 78, 60, -14.6 },
    { 90, 60, 0 },
    { 90, 120, 0 },
    { 183, -95, 200 },
    { 180, -90, 13 },
    { -200, 232, -176 },
    { 70, 34, 41 },
    { 90, -165, 0 },
    { 90, 0, 0 },
    { 90, 75, 0 }
  };

  // Generate safe password
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution d1(0, 1);
  std::uniform_int_distribution d2(0, 3);
  std::uniform_int_distribution d3(0, 9);
  std::uniform_real_distribution d4(0.f, 360.f);

  std::shuffle(pencilOrder.begin(), pencilOrder.end(), gen);

  std::vector<int> posIndices(14);
  std::iota(posIndices.begin(), posIndices.end(), 0);
  std::shuffle(posIndices.begin(), posIndices.end(), gen);
  int pencilPosIdx = 0;

  int pictureSet = d2(gen);
  black = 1;  // Black = number of pictures
  if (pictureSet == 1) {
    black = 5;
  } else if (pictureSet == 2) {
    black = 8;
  } else if (pictureSet == 3) {
    black = 10;
  }

  int statue = d1(gen);
  white = 0;
  int rgbSum = 0;
  int yapSum = 0;
  password = "";
  for (int i = 0; i < 7; i++) {
    int n = d3(gen);
    Pencil color = static_cast<Pencil>(pencilOrder[i]);
    if (color == Red || color == Green || color == Blue) {
      rgbSum += n;
    } else if (i == Yellow || i == Azure || i == Purple) {
      yapSum += n;
    }
    password += std::to_string(n);
  }

  if (statue == 0) white = rgbSum;
  else white = yapSum;

  password += std::to_string(black) + std::to_string(white);

  glm::vec3 angle;
  for (int i = 0; i < 3; i++) angle[i] = glm::radians(d4(gen));

  sf::SoundBuffer correctBuffer, failBuffer;
  if (correctBuffer.loadFromFile(getResourcePath("sounds/correct.mp3"))) {
    AudioManager::instance().loadSound("correct", correctBuffer);
  }
  if (failBuffer.loadFromFile(getResourcePath("sounds/fail.mp3"))) {
    AudioManager::instance().loadSound("fail", failBuffer);
  }

  std::filesystem::path file = getResourcePath("objects.json");
  if (file.empty()) return false;

  using json = nlohmann::json;
  std::ifstream in(file);

  in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  json j;

  try {
    in >> j;
  } catch (std::exception &e) {
    std::cerr << "ERROR: failed to read game data from " << file << ": "
              << e.what() << std::endl;
    return false;
  }

  if (!j.contains("objects")) return false;
  json &s = j["objects"];

  for (const auto &entry : s) {
    std::shared_ptr<Object> obj = std::make_shared<Object>(entry);

    if (entry.contains("animation")) {
      obj->animation = entry["animation"].get<Animation>();
    }

    std::string sceneName = entry.value("scene", "room");
    std::string name = entry.value("name", "");

    // Handle statues: both are created (pushed in objects), but only the chosen one is placed on canvas.
    if (name.find("Statue") != std::string::npos) {
      if ((statue == 0 && obj->name == "Statue0") ||
          (statue == 1 && obj->name == "Statue1")) {
        obj->setRotation(angle);
        canvas.objs.push_back(obj.get());
      }
      // Always keep track  existing objects, but ignore room/inventory.
      objects.push_back(obj);
      continue;
    }

    // Load only the selected pictures according to pictureSet
    if (name.find("Pictures") != std::string::npos) {
      if (pictureSet >= 1 && obj->name == "Pictures1") {
        if (sceneName == "room")
          room.objs.push_back(obj.get());
        else if (sceneName == "canvas")
          canvas.objs.push_back(obj.get());
        else if (sceneName == "inventory")
          inventory.objs.push_back(obj.get());
      }
      if (pictureSet >= 2 && obj->name == "Pictures2") {
        if (sceneName == "room")
          room.objs.push_back(obj.get());
        else if (sceneName == "canvas")
          canvas.objs.push_back(obj.get());
        else if (sceneName == "inventory")
          inventory.objs.push_back(obj.get());
      }
      if (pictureSet >= 3 && obj->name == "Pictures3") {
        if (sceneName == "room")
          room.objs.push_back(obj.get());
        else if (sceneName == "canvas")
          canvas.objs.push_back(obj.get());
        else if (sceneName == "inventory")
          inventory.objs.push_back(obj.get());
      }
      objects.push_back(obj);
      continue;
    }

    // Handle random pencil positions
    if (name.find("Pencil") != std::string::npos &&
        name.find("_Placeholder") == std::string::npos &&
        name.find("_Placed_On_") == std::string::npos) {
      if (pencilPosIdx < posIndices.size()) {
        int idx = posIndices[pencilPosIdx++];
        obj->setPosition(pencilPositions[idx]);
        obj->setRotation(glm::radians(pencilRotations[idx]));
      }
    }

    // Brushes: if they belong to the room we intentionally DON'T add them now (they spawn later).
    if (obj->name.find("Brush") != std::string::npos && sceneName == "room") {
      // skip adding to room (but keep in objects)
      objects.push_back(obj);
      continue;
    }

    if (name.find("_Placeholder") != std::string::npos) {
      std::string pencilName = name.substr(0, name.find("_Placeholder"));
      const std::string colorNames[] = {"Red", "Orange", "Yellow", "Green", "Azure", "Blue", "Purple"};
      
      for (int i = 0; i < 7; i++) {
        if (pencilName.find(colorNames[pencilOrder[i]]) != std::string::npos) {
          obj->setPosition(placeholderPositions[i]);
          obj->setRotation(glm::radians(glm::vec3(-90, 90, 0)));
          break;
        }
      }
    }

    // General case: place into the scene indicated by sceneName (default room).
    if (sceneName == "room") {
      room.objs.push_back(obj.get());
    } else if (sceneName == "inventory") {
      inventory.objs.push_back(obj.get());
    } else if (sceneName == "canvas") {
      canvas.objs.push_back(obj.get());
    } else {
      // Fallback: put in room
      room.objs.push_back(obj.get());
    }

    objects.push_back(obj);
  }

  // Re-assign parents, searching in the correct scene collection according to entry["scene"]
  for (const auto &entry : s) {
    if (entry.contains("parent")) {
      std::string objName = entry["name"];
      std::string sceneName = entry.value("scene", "room");
      std::string parentName = entry["parent"];
      if (!parentName.empty() && parentName != objName) {
        // choose the scene vector to search
        std::vector<Object*> *sceneObjs = &room.objs;
        if (sceneName == "inventory") sceneObjs = &inventory.objs;
        else if (sceneName == "canvas") sceneObjs = &canvas.objs;

        for (auto &obj : *sceneObjs) {
          if (obj->name == objName) {
            for (auto &parent : *sceneObjs) {
              if (parent->name == parentName) {
                obj->parent = parent;
                break;
              }
            }
            break;
          }
        }
      }
    }
  }

  // Ensure inventoryIndex reflects actual inventory contents
  if (inventory.objs.empty()) {
    inventoryIndex = -1;
  } else {
    // default to first item
    inventoryIndex = 0;
  }

  return true;
}

void Game::loadLeaderboard() {
  using json = nlohmann::json;
  leaderboard.clear();
  std::ifstream f(getResourcePath("leaderboard.json", true));
  if (!f.is_open()) return;
  json j;
  if (f.peek() != std::ifstream::traits_type::eof()) {
    f >> j;
  }
  f.close();

  if (!j.is_array()) j = nlohmann::json::array();

  for (const auto &entry : j) {
    std::string name = entry.value("name", std::string("Anonymous"));
    int score = entry.value("score", 0);
    leaderboard.emplace_back(name, score);
  }
  std::sort(
      leaderboard.begin(), leaderboard.end(),
      [](const std::pair<std::string, int> &a,
         const std::pair<std::string, int> &b) { return a.second > b.second; });
}

void Game::drawLeaderboard() {
  renderer->drawText("Leaderboard:", static_cast<float>(fbWidth) * 0.05f,
                     static_cast<float>(fbHeight) * 0.2f, 1.0f,
                     glm::vec3(0.95f));
  for (int i = 0; i < leaderboard.size(); i++) {
    auto &entry = leaderboard[i];
    renderer->drawText(entry.first + ":", static_cast<float>(fbWidth) * 0.05f,
                       static_cast<float>(fbHeight) * (0.28f + i * 0.08f), 0.8f,
                       glm::vec3(1.0f));
    renderer->drawText(std::to_string(entry.second),
                       static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * (0.28f + i * 0.08f), 0.8f,
                       glm::vec3(1.0f, 1.0f, 0.0f));
  }
}

void Game::drawCredits(float scroll) {
  const char *credits[] = {"Programming",
                           "Graphics & Core - Salvatore Martorana",
                           "Text - Emma Toutel",
                           "Audio - Mattia Briguglio",
                           "",
                           "3D Models",
                           "Room - Salvatore Martorana",
                           "Furniture & Props - Emma Toutel",
                           "Furniture & Props - Mattia Briguglio",
                           "",
                           "Textures & Materials",
                           "ambientCG",
                           "Poly Haven",
                           "Texture Labs",
                           "Sketchfab",
                           "",
                           "",
                           "Thank you for playing!"};
  renderer->drawText("Escape Room", static_cast<float>(fbWidth) * 0.5f,
                     static_cast<float>(fbHeight) * (0.25f - scroll), 1.5f,
                     glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
  for (int i = 0; i < sizeof(credits) / sizeof(credits[0]); i++) {
    renderer->drawText(
        credits[i], static_cast<float>(fbWidth) * 0.5f,
        static_cast<float>(fbHeight) * (0.5f + i * 0.08f - scroll), 0.8f,
        glm::vec3(1.0f), Align::Center);
  }
}

void Game::update() {
  if (state == Splashscreen || state == GameOver || state == Credits) return;

  time += deltaTime;
  remainingTime -= deltaTime;
  if (remainingTime <= 0.f) {
    AudioManager::instance().playSound("fail");
    state = GameOver;
  }

  if (state == InGame) {
    // Calculate the amount of the movement considering sprint (SHIFT)
    float velocity = playerSpeed * deltaTime;
    if (input.shift) velocity *= SPRINT;

    if (input.w) player.move(player.front, velocity, !debug);
    if (input.s) player.move(-player.front, velocity, !debug);
    if (input.a) player.move(-player.right, velocity, !debug);
    if (input.d) player.move(player.right, velocity, !debug);

    for (auto &obj : objects) {
      obj->onUpdate();
    }

    if (equippedObj) {
      equippedObj->setPosition(player.position + player.front * 0.8f +
                               player.right * 0.2f - player.up * 0.2f);
      equippedObj->setRotation(player.orientation);
    }

    if (brushesTimer > 0.f) {
      brushesTimer -= deltaTime;
      if (brushesTimer <= 0.f) {
        brushesTimer = 0.f;
        // Clear brushes from room if they weren't collected
        for (auto it = room.objs.begin(); it != room.objs.end();) {
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
        Object *obj = *roomObjIt;
        bool erased = false;

        if (brushesTimer > 0 &&
            obj->name.find("Brush") != std::string::npos) {
          if (obj->checkCollision(player)) {
            roomObjIt = room.objs.erase(roomObjIt);
            collectedBrushes++;
            AudioManager::instance().playSound("CollectItem", 80.f, false);
            erased = true;

            if (collectedBrushes >= 4) {
              brushesTimer = 0.f;  // Success
              brushesCompleted = true;
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

    if (inventoryIndex < 0 || inventoryIndex >= (int)inventory.objs.size()) {
      inventoryIndex = 0;
    }

    if (input.w)
      inventory.objs[inventoryIndex]->rotate(Object::WRLD_RIGHT, -amount, true);
    if (input.s)
      inventory.objs[inventoryIndex]->rotate(Object::WRLD_RIGHT, amount, true);
    if (input.a)
      inventory.objs[inventoryIndex]->rotate(Object::WRLD_UP, -amount, true);
    if (input.d)
      inventory.objs[inventoryIndex]->rotate(Object::WRLD_UP, amount, true);
  } else if (state == Canvas) {
    // Portal view rotation in Puzzle mode
    float amount = glm::radians(45.f) * deltaTime;
    if (input.shift) amount *= 0.2f;

    for (const auto &obj : canvas.objs) {
      if (input.w) obj->rotate(Object::WRLD_RIGHT, -amount, true);
      if (input.s) obj->rotate(Object::WRLD_RIGHT, amount, true);
      if (input.a) obj->rotate(Object::WRLD_UP, -amount, true);
      if (input.d) obj->rotate(Object::WRLD_UP, amount, true);
    }
  }
}

void Game::draw() {
  if (state == Splashscreen) {
    Renderer::clear();
    drawLeaderboard();
    renderer->drawText(
    "Steal the beautiful painting", static_cast<float>(fbWidth) * 0.95f,
    static_cast<float>(fbHeight) * 0.3f, 1.0f, glm::vec3(1.0f),
    Align::Right);
    renderer->drawText(
    "before time runs out", static_cast<float>(fbWidth) * 0.95f,
    static_cast<float>(fbHeight) * 0.4f, 1.0f, glm::vec3(1.0f),
    Align::Right);
    renderer->drawText(
    "Put all pencils correctly", static_cast<float>(fbWidth) * 0.95f,
    static_cast<float>(fbHeight) * 0.6f, 1.0f, glm::vec3(1.0f),
    Align::Right);
    renderer->drawText(
  "to get a bonus score", static_cast<float>(fbWidth) * 0.95f,
  static_cast<float>(fbHeight) * 0.7f, 1.0f, glm::vec3(1.0f),
  Align::Right);
    renderer->drawText(
        "Press any key to start...", static_cast<float>(fbWidth) * 0.95f,
        static_cast<float>(fbHeight) * 0.95f, 1.2f, glm::vec3(1.0f, 1.0f, 0.0f),
        Align::Right);
    return;
  }
  if (state == GameOver) {
    Renderer::clear();
    renderer->drawText("GAME OVER", static_cast<float>(fbWidth) * 0.5f,
                     static_cast<float>(fbHeight) * 0.4f, 2.0f,
                     glm::vec3(1.0f, 0.0f, 0.0f), Align::Center);
    renderer->drawText("The police caught you!", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.55f, 1.2f,
                       glm::vec3(1.0f), Align::Center);
    renderer->drawText("Press [Esc] to quit",
                       static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.8f, 1.0f,
                       glm::vec3(0.6f), Align::Center);
    return;
  }
  if (state == LeaderboardEntry) {
    Renderer::clear();
    renderer->drawText("CONGRATULATIONS!", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.25f, 1.5f,
                       glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
    renderer->drawText("Enter your name:", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.4f, 1.0f,
                       glm::vec3(1.0f), Align::Center);
    renderer->drawText(playerName + "_", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.5f, 1.2f,
                       glm::vec3(1.0f), Align::Center);
    renderer->drawText("Press [Enter] to save",
                       static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.75f, 1.0f,
                       glm::vec3(1.0f), Align::Center);
    return;
  }
  static auto canvasTex =
      dynamic_cast<DynamicTexture *>(renderer->getTexture("#Canvas"));
  if (brushesCompleted) {
    renderer->updateTexture(*canvasTex, canvas);
  } else {
    // Render just the background as an empty scene
    renderer->updateTexture(*canvasTex,
                            {{}, canvas.cam, {}, canvas.clearColor});
  }
  // Scene rendering
  if (state == InGame || state == Canvas) {
    int id = renderer->readObjFromCursor(room);
    hoveredObj = findObject(id);

    if (state == Canvas && canvasTex)
      renderer->updateTexture(*canvasTex, canvas);
    renderer->drawScene(room);

    // --- 3D Text ---

    if (paperIsNotPicked) {
      auto textModel =
    glm::translate(glm::mat4(1.0f), glm::vec3(-0.21508, 0.43257, -2.6));
      textModel =
          glm::rotate(textModel, glm::radians(-90.0f), glm::vec3(1, 0, 0));

      renderer->drawText3D("Click me!", textModel, player, 0.4f,
                           glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
    }

    if (!safeUnlocked) {
      auto textModel =
          glm::translate(glm::mat4(1.0f), glm::vec3(1.952, 1.39983, 0.82));
      textModel =
          glm::rotate(textModel, glm::radians(90.0f), glm::vec3(0, 1, 0));

      renderer->drawText3D(enteredCode, textModel, player, 0.33f,
                           glm::vec3(0.0f, 0.8f, 0.0f));
    }

    // --- HUD ---

    // Crosshair
    renderer->drawText(".", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.5f, 1.0f,
                       glm::vec3(1.0f), Align::Center);

    glm::vec3 remainingTimeColor = glm::vec3(1.0f);

    // After 5 minutes make text more red
    if (remainingTime <= 0.5f * maxTime) {
      float progress =
          1.f - glm::clamp((remainingTime) / (0.5f * maxTime), 0.f, 1.f);
      remainingTimeColor =
          (1 - progress) * glm::vec3(1.0f) +
          progress * glm::vec3(0.8f, 0.0f, 0.f);  // From -4 to +2 semitones
    }

    renderer->drawText(
        "Police: " + std::to_string(static_cast<int>(remainingTime) / 60) +
            "min " + std::to_string(static_cast<int>(remainingTime) % 60) + "s",
        static_cast<float>(fbWidth) * 0.05f,
        static_cast<float>(fbHeight) * 0.1f, 1.0f, remainingTimeColor);

    if (brushesTimer > 0.f) {
      renderer->drawText(
          "Brushes collected: (" + std::to_string(collectedBrushes) + "/4)",
          static_cast<float>(fbWidth) * 0.05f,
          static_cast<float>(fbHeight) * 0.2f, 1.0f, glm::vec3(1.0f));
      renderer->drawText(
          "Time left: " + std::to_string(static_cast<int>(brushesTimer)) +
              "s",
          static_cast<float>(fbWidth) * 0.7f,
          static_cast<float>(fbHeight) * 0.2f, 1.0f, glm::vec3(1.0f));
    }

    // Contextual hints
    if (!hoveredObj) return;

    if (brushesCompleted && hoveredObj->name == "Canvas") {
      renderer->drawText("Align",
                         static_cast<float>(fbWidth) * 0.5f,
                         static_cast<float>(fbHeight) * 0.95f, 1.2f,
                         glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
    }

    if (!brushesCompleted && hoveredObj->name == "Palette") {
      renderer->drawText(
          "Collect all brushes in time!",
          static_cast<float>(fbWidth) * 0.5f,    // right side
          static_cast<float>(fbHeight) * 0.8f,  // vertical placement
          1.0f, glm::vec3(1.0f), Align::Center);
      renderer->drawText(
          "Start",
          static_cast<float>(fbWidth) * 0.5f,    // right side
          static_cast<float>(fbHeight) * 0.95f,  // vertical placement
          1.2f, glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
    }

    if (hoveredObj->name == "Picture") {
      renderer->drawText(
          "Is that a capybara?",
          static_cast<float>(fbWidth) * 0.5f,    // right side
          static_cast<float>(fbHeight) * 0.8f,  // vertical placement
          1.0f, glm::vec3(1.0f), Align::Center);
      renderer->drawText(
          "Pick",
          static_cast<float>(fbWidth) * 0.5f,    // right side
          static_cast<float>(fbHeight) * 0.95f,  // vertical placement
          1.2f, glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
    }
    if (hoveredObj->name == "Door") {
      if (!doorUnlocked) {
        if (equippedObj && equippedObj->name.find("Key") != std::string::npos) {
          renderer->drawText(
              "Unlock",
              static_cast<float>(fbWidth) * 0.5f,    // right side
              static_cast<float>(fbHeight) * 0.95f,  // vertical placement
              1.2f, glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
        } else {
          renderer->drawText(
              "Door is locked",
              static_cast<float>(fbWidth) * 0.5f,    // right side
              static_cast<float>(fbHeight) * 0.95f,  // vertical placement
              1.0f, glm::vec3(1.0f), Align::Center);
        }
      }
    }

    // Contextual Action hints
    std::string hint;
    if (hoveredObj->name.find("_Placed_On_") != std::string::npos) {
      if (!equippedObj) {
        hint = "Pick";
      } else if (equippedObj->name.find("Pencil") != std::string::npos) {
        hint = "Swap";
      }
    } else if (hoveredObj->name.find("_Placeholder") != std::string::npos) {
      if (equippedObj && equippedObj->name.find("Pencil") != std::string::npos) {
        hint = "Place";
      }
    } else if (hoveredObj->name.find("Pencil") != std::string::npos ||
               hoveredObj->name.find("Key") != std::string::npos) {
      hint = "Pick";
    }

    if (!hint.empty()) {
      renderer->drawText(hint, static_cast<float>(fbWidth) * 0.5f,
                         static_cast<float>(fbHeight) * 0.95f, 1.2f,
                         glm::vec3(1.0f, 1.0f, 0.0f), Align::Center);
    }
  } else if (state == Inventory) {
    renderer->drawScene(inventory, false, inventoryIndex);

    if (inventoryIndex != -1) {
      Object *selected = inventory.objs[inventoryIndex];
      renderer->drawText(selected->name, static_cast<float>(fbWidth) * 0.5f,
                         static_cast<float>(fbHeight) * 0.95f, 1.0f,
                         glm::vec3(1.0f), Align::Center);

      std::string colorNames[] = {"Red", "Orange", "Yellow", "Green", "Azure", "Blue", "Purple"};
      if (selected->name == "Picture") {
        static Object text;
        text.setPosition(selected->position + selected->front * +0.015f);
        text.setRotation(selected->rotation);
        text.setScale(glm::vec3(-1.0f, 1.0f, 1.0f));

        for (int i = 0; i < 7; i++) {
          for (int j = 0; j < 7; j++) {
            if (i == pencilOrder[j]) {
              text.setPosition(selected->position + selected->front * 0.01f + selected->up * (0.038f * (4 - i)));
              renderer->drawText3D(colorNames[i] + " = " + password[j], text.getModelMatrix(), inventoryView, 0.33f,
                                   glm::vec3(0.8f), Align::Center);
              break;
            }
          }
        }
        text.setPosition(selected->position + selected->front * 0.01f + selected->up * (0.038f * -3));
        renderer->drawText3D("White is last", text.getModelMatrix(), inventoryView, 0.33f,
                             glm::vec3(0.8f), Align::Center);
        text.setPosition(selected->position + selected->front * 0.01f + selected->up * (0.038f * -4));
        renderer->drawText3D("Black is second-last", text.getModelMatrix(), inventoryView, 0.33f,
                             glm::vec3(0.8f), Align::Center);
      }

      if (selected->name == "Paper") {
        static Object text;

        // Front side
        text.setPosition(selected->position);
        text.setRotation(selected->rotation);
        text.setScale(glm::vec3(1.0f, 1.0f, 1.0f));

        std::string frontStrings[] = {"Controls:", "[WASD] Move", "[Left Mouse] Interact", "[Right Mouse] Equip", "[Tab] Inventory", "Black = #Frames", "White = Color Sum"};

        for (int i = 0; i < 7; i++) {
          text.setPosition(selected->position + selected->front * -0.002f + selected->up * (0.04f * (3 - i)));
          renderer->drawText3D(frontStrings[i], text.getModelMatrix(), inventoryView, 0.33f, glm::vec3(0.1f), Align::Center);
        }

        // Back side
        text.setPosition(selected->position);
        text.setRotation(selected->rotation);
        text.setScale(glm::vec3(-1.0f, 1.0f, 1.0f));

        for (int i = 0; i < 7; i++) {
          text.setPosition(selected->position + selected->front * 0.002f + selected->up * (0.04f * (3 - i)));
          renderer->drawText3D("#" + std::to_string(i + 1) + " " + colorNames[pencilOrder[i]], text.getModelMatrix(),
                               inventoryView, 0.33f, glm::vec3(0.1f), Align::Center);
        }
      }

      if (inventory.objs.size() > 1) {
        renderer->drawText("[Q] Previous       [E] Next",
                           static_cast<float>(fbWidth) * 0.5f,
                           static_cast<float>(fbHeight) * 0.1f, 0.8f,
                           glm::vec3(0.6f), Align::Center);
      }
    }
  } else if (state == ExitDialog) {
    Renderer::clear();
    renderer->drawText("Are you sure you want to exit?", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.4f, 1.2f,
                       glm::vec3(1.0f), Align::Center);
    renderer->drawText("[Enter] Yes          [Esc] No", static_cast<float>(fbWidth) * 0.5f,
                       static_cast<float>(fbHeight) * 0.6f, 1.0f,
                       glm::vec3(0.8f, 0.8f, 0.0f), Align::Center);
    return;
  } else if (state == Credits) {
    static float creditTime = 0.f;
    float scroll = 0.05f * creditTime;
    Renderer::clear();
    drawCredits(scroll);
    creditTime += deltaTime;
  }
}

Object *Game::getObject(const std::string &name) {
  for (auto &obj : objects) {
    if (obj.get()->name == name) return obj.get();
  }
  return nullptr;
}

// TODO: Optimize with hashmap.
Object *Game::findObject(int id) {
  if (id < 1) return nullptr;
  for (auto &obj : objects) {
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
          safeUnlocked = true;
          if (auto door = getObject("Safe Door")) door->animation.play();
        } else {
          safeTries++;
        }
        enteredCode.clear();
      }
    }
    return;
  }

  pos = obj->name.find("Palette");
  if (pos != std::string::npos) {
    if (brushesCompleted) return;

    brushesTimer = 10.f;
    collectedBrushes = 0;
    brushesTries++;

    std::vector<std::string> brushNames = {"Brush0", "Brush1", "Brush2",
                                           "Brush3"};
    for (const auto &name : brushNames) {
      Object *brush = getObject(name);
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

  pos = obj->name.find("Canvas");
  if (pos != std::string::npos) {
    if (!brushesCompleted) return;
    lastPos = player.position;
    lastRot = player.rotation;
    player.setPosition(obj->position - obj->front * 1.5f);
    player.setRotation(obj->rotation);
    state = Canvas;
    return;
  }

  if (obj->name == "Safe Door") return;  // Do not open the door

  if (obj->name == "Door") {
    if (!doorUnlocked && equippedObj &&
        equippedObj->name.find("Key") != std::string::npos) {
      doorUnlocked = true;
      obj->animation.play();
      AudioManager::instance().playSound("OpenDoor");
    }
    return;
  }
  pos = obj->name.find("MacGuffin");
  if (pos != std::string::npos) {
    playerName = "";
    score = static_cast<int>(remainingTime * 100) + pencilsCorrect * 1000 - safeTries * 100;
    if (brushesTries > 1) score -= brushesTries * 100;
    state = LeaderboardEntry;
    return;
  }

  // Used mostly for opening/closing objects
  if (!obj->animation.empty()) {
    obj->animation.toggle();
    return;
  }

  pos = obj->name.find("_Placed_On_");
  if (pos != std::string::npos) {
    std::string pencilName = obj->name.substr(0, pos);
    std::string placeholderName = obj->name.substr(pos + 11);
    Object *placeholder = getObject(placeholderName);

    if (!equippedObj) {
      // Pick item and restore placeholder
      placeholder->setPosition(obj->position);
      placeholder->setRotation(obj->rotation);
      room.objs.push_back(placeholder);

      obj->name = pencilName;
      pick(obj);
    } else if (equippedObj->name.find("Pencil") != std::string::npos) {
      // Swap
      Object *equipped = equippedObj;
      std::string newPencilName = equipped->name;

      // Pick the one on the table
      obj->name = pencilName;
      pick(obj);

      // Place the equipped one
      place(equipped, placeholder);
    }
    return;
  }

  pos = obj->name.find("_Placeholder");
  if (pos != std::string::npos) {
    if (equippedObj && equippedObj->name.find("Pencil") != std::string::npos) {
      place(equippedObj, obj);
    }
    return;
  }

  pick(obj);
}

// --- Callbacks ---

void Game::equip(Object *obj) {
  if (!obj) {
    // Unequip current
    if (equippedObj) {
      equippedObj->setPosition();
      equippedObj->setRotation();
      inventory.objs.push_back(equippedObj);

      // Remove from room if present
      room.objs.erase(
          std::remove(room.objs.begin(), room.objs.end(), equippedObj),
          room.objs.end());

      equippedObj = nullptr;
      inventoryIndex = static_cast<int>(inventory.objs.size()) - 1;
      if (inventoryIndex < 0) inventoryIndex = -1;
    }
  } else {
    if (obj->name == "Paper" || obj->name == "Picture") return; // Cannot be equipped

    // Equip new
    if (equippedObj) equip(nullptr);

    equippedObj = obj;
    room.objs.push_back(equippedObj);
    inventory.objs.erase(
        std::remove(inventory.objs.begin(), inventory.objs.end(), obj),
        inventory.objs.end());

    if (inventory.objs.empty()) {
      inventoryIndex = -1;
    } else {
      if (inventoryIndex >= (int)inventory.objs.size())
        inventoryIndex = static_cast<int>(inventory.objs.size()) - 1;
    }
  }
}

void Game::pick(Object *obj) {
  if (!obj) return;

  if (obj->name == "Paper") {
    paperIsNotPicked = false;
  }

  // Add to inventory
  inventory.objs.push_back(obj);

  // Remove from room
  room.objs.erase(std::remove(room.objs.begin(), room.objs.end(), obj),
                  room.objs.end());

  obj->setPosition();
  obj->setRotation();
  inventoryIndex = static_cast<int>(inventory.objs.size()) - 1;
  if (inventoryIndex < 0) inventoryIndex = -1;
  state = Inventory;
  Application::setBackgroundLowpass(true);
}

void Game::place(Object *obj, Object *placeholder) {
  if (!obj || !placeholder) return;

  std::string pencilName = obj->name;
  obj->setPosition(placeholder->position);
  obj->setRotation(placeholder->rotation);
  obj->name = pencilName + "_Placed_On_" + placeholder->name;

  room.objs.push_back(obj);

  // Remove placeholder from room
  room.objs.erase(std::remove(room.objs.begin(), room.objs.end(), placeholder),
                  room.objs.end());

  // Remove object from inventory or equipped
  inventory.objs.erase(
      std::remove(inventory.objs.begin(), inventory.objs.end(), obj),
      inventory.objs.end());

  if (equippedObj == obj) equippedObj = nullptr;

  checkPencils();
}

void Game::checkPencils() {
  int correct = 0;
  for (auto &obj : objects) {
    size_t pos = obj->name.find("_Placed_On_");
    if (pos != std::string::npos) {
      std::string pencilName = obj->name.substr(0, pos);
      std::string placeholderName = obj->name.substr(pos + 11);
      if (placeholderName == pencilName + "_Placeholder") {
        correct++;
      }
    }
  }
  if (correct == 7) {
    AudioManager::instance().playSound("correct");
    pencilsCorrect = true;
  } else {
    pencilsCorrect = false;
  }
}

void Game::onKey() {
  if (state == Splashscreen) {
    for (auto k : input.keys)
      if (k) state = InGame;
  }
  if (state == GameOver) {
      if (input.esc) close = true;
  }
  if (state == InGame) {
    if (input.tab) {
      state = Inventory;
      Application::setBackgroundLowpass(true);
    }
    if (input.esc) {
      state = ExitDialog;
    }
  } else if (state == Inventory) {
    if (input.esc || input.tab) {
      state = InGame;
      Application::setBackgroundLowpass(false);
    }

    if (input.q && !inventory.objs.empty()) {
      inventoryIndex = (inventoryIndex - 1 + (int)inventory.objs.size()) %
                       (int)inventory.objs.size();
    }
    if (input.e && !inventory.objs.empty()) {
      inventoryIndex = (inventoryIndex + 1) % (int)inventory.objs.size();
    }
  } else if (state == Canvas) {
    if (input.esc) {
      state = InGame;
      player.setPosition(lastPos);
      player.setRotation(lastRot);
    }
  } else if (state == ExitDialog) {
    if (input.esc) {
      state = InGame;
    }
    if (input.enter) {
      close = true;
    }
  } else if (state == LeaderboardEntry) {
    if (input.backspace && !playerName.empty()) {
      playerName.pop_back();
      input.backspace = false;  // Prevent multiple pops per press if not
                                // careful, though keyCallback handles it
    }
    if (input.enter) {
      using json = nlohmann::json;
      // Save to leaderboard.json
      json j;
      std::ifstream f(getResourcePath("leaderboard.json"), true);
      if (f.is_open() && f.peek() != std::ifstream::traits_type::eof()) {
        f >> j;
      }
      f.close();

      if (!j.is_array()) {
        j = nlohmann::json::array();
      }

      j.push_back({{"name", playerName.empty() ? "Anonymous" : playerName},
                   {"score",score}});

      std::ofstream o(getResourcePath("leaderboard.json"));
      o << j.dump(4) << std::endl;
      o.close();

      state = Credits;
      input.enter = false;
    }
  }
  if (state == Credits) {
    if (input.esc) {
      close = true;
    }
  }
}

void Game::onMouseButton() {
  if (state == InGame) {
    if (input.rmb) {
      if (equippedObj) {
        equip(nullptr);
      } else if (!inventory.objs.empty()) {
        // Ensure inventoryIndex is valid
        if (inventoryIndex < 0 || inventoryIndex >= (int)inventory.objs.size()) {
          inventoryIndex = 0;
        }

        equip(inventory.objs[inventoryIndex]);
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
    // Guard accesses: do nothing if inventory is empty / no selection
    if (!inventory.objs.empty() && inventoryIndex != -1 &&
        inventoryIndex < (int)inventory.objs.size()) {
      inventory.objs[inventoryIndex]->rotate(
          inventoryView.up, -glm::radians(mouseSensitivity * xdelta), true);
      inventory.objs[inventoryIndex]->rotate(
          inventoryView.right, -glm::radians(mouseSensitivity * ydelta), true);
    }
  } else if (state == Canvas) {
    player.yaw(0.1f * glm::radians(mouseSensitivity * xdelta));
    player.pitch(0.1f * glm::radians(mouseSensitivity * ydelta));

    canvasView.yaw(0.05f * glm::radians(mouseSensitivity * xdelta));
    canvasView.pitch(0.05f * glm::radians(mouseSensitivity * ydelta));
  }
}

void Game::onMouseScroll(float yoffset) {
  float amount = glm::radians(3.f * yoffset);
  if (state == InGame) {
    player.zoom(amount);
  }
  if (state == Inventory) {
    inventoryView.zoom(amount);
  }
}

void Game::onResize(int width, int height) {
  float aspect = static_cast<float>(width) / static_cast<float>(height);
  player.setAspect(aspect);
  inventoryView.setAspect(aspect);
}

void Game::onChar(unsigned int codepoint) {
  if (state == LeaderboardEntry && playerName.size() < 20) {
    if (codepoint < 128) {
      playerName += static_cast<char>(codepoint);
    }
  }
}
