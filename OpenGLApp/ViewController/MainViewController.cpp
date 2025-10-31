#include "MainViewController.h"

void MainViewController::loadModels() {
    // You can use this function to preload the 3D models for your game, to use in another ViewController

}

int MainViewController::main(GLFWwindow* window)
{
    // Shader Loading
    shaderText = Shader("../resources/Shaders/shaderText.vs", "../resources/Shaders/shaderText.fs");
    gameShader = Shader("../resources/Shaders/shader.vs", "../resources/Shaders/shader.fs");
    menuBGShader = Shader("../resources/Shaders/2DImage.vs", "../resources/Shaders/2DImage.fs");

    sprite2D = SpriteRenderer(menuBGShader);

    Texture2D backGroundTexture = loadTextureFromFile("../resources/Backgrounds/templateBackground.png",false);

    Texture2D startButtonTexture = loadTextureFromFile("../resources/Buttons/StartButton/unselected.png", true);
    Texture2D startButtonTextureSelected = loadTextureFromFile("../resources/Buttons/StartButton/selected.png", true);

    startButton = Button(0.0f, 0.0f, 0.0f, 0.0f);

    menuMusic.playSound();

    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        // render
        // ------

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Background
        menuBGShader.use();
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT), 0.0f, -1.0f, 1.0f);
        menuBGShader.setMat4("projection", projection);

        sprite2D.DrawSprite(backGroundTexture, glm::vec2(0.0f, 0.0f), glm::vec2(static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)));

        //Start Button
        const float y = static_cast<float>(SCR_HEIGHT) - static_cast<float>(SCR_HEIGHT) * 0.25f;
        const float buttonWidth = static_cast<float>(SCR_WIDTH) / 5.0f;
        const float buttonHeight = buttonWidth / 4.0f;
        startButton.x = static_cast<float>(SCR_WIDTH) / 2 - buttonWidth*1.4f / 2;
        startButton.y = y - buttonHeight*0.4f/2;
        startButton.height = buttonHeight*1.4f;
        startButton.width = buttonWidth*1.4f;
        if (startButton.selected == false) {
            sprite2D.DrawSprite(startButtonTexture, glm::vec2(startButton.x, startButton.y), glm::vec2(startButton.width, startButton.height));
        }
        else {
            sprite2D.DrawSprite(startButtonTextureSelected, glm::vec2(startButton.x, startButton.y), glm::vec2(startButton.width, startButton.height));
        }

        if (loaded == false) {
            std::cout << "Load Models" << std::endl;
            loadModels();
            loaded = true;
        }
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------

    glfwTerminate();
	return 0;
}
