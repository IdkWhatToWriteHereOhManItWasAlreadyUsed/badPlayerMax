#include "Game.h"
#include <exception>
#include <thread>

#ifdef BOJE_CHEL
#include <windows.h>
#include <psapi.h>
#endif 

#include <renderer/Frustum.h>
#include <renderer/Shader.h>
#include <videoWorld/chunk/generation/ChunkGenerator.h>
#include <videoWorld/chunk/rendering/RayCaster.h>
#include <videoWorld/block/BlocksManager.h>
#include <imgui/imgui.h>
#include "InputManager.h"
#include <imgui/imgui_impl_glfw.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/fwd.hpp>
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <renderer/lighting/Lights.h>

void Game::Run()
{
    MainLoop();
}

void Game::MainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        ProcessInput();

        UpdateTiming();
        UpdatePerformanceStats();
        UpdateCameraMatrices();

        RenderWorld();
        //RenderCrosshair();

        RenderUI();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Cleanup();
}

void Game::UpdateTiming()
{
    float currentFrame = glfwGetTime();
    m_state.deltaTime = currentFrame - m_state.lastFrame;
    m_state.lastFrame = currentFrame;
}

void Game::UpdatePerformanceStats()
{
    static double lastFPSTime = 0.0;
    static int frameCount = 0;
    static size_t initialMemory = 0;

    double currentFrame = glfwGetTime();
    frameCount++;

    double timeSinceLastFPS = currentFrame - lastFPSTime;
    if (timeSinceLastFPS >= 1.0)
    {
        m_state.displayedFPS = static_cast<float>(frameCount) / static_cast<float>(timeSinceLastFPS);
        frameCount = 0;
        lastFPSTime = currentFrame;

#ifdef BOJE_CHEL
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        {
            if (initialMemory == 0)
                initialMemory = pmc.WorkingSetSize;

            SIZE_T currentMemory = pmc.WorkingSetSize;
            m_state.memoryMB = static_cast<float>(currentMemory) / (1024.0f * 1024.0f);
            m_state.memoryChangeMB = static_cast<float>(currentMemory - initialMemory) / (1024.0f * 1024.0f);
        }
#endif
    }
}

void Game::UpdateCameraMatrices()
{
    glm::mat4 projection = glm::perspective
    (
        glm::radians(camera.Zoom),
        static_cast<GLfloat>(m_state.windowWidth) / static_cast<GLfloat>(m_state.windowHeight),
        0.1f,
        1500.0f
    );

    glm::mat4 view = camera.GetViewMatrix();

    frustum.update(projection * view);
}

RayCastResult Game::PerformRayCast() const
{
    glm::vec3 rayStart = camera.Position;
    glm::vec3 rayDir = camera.Front;
    return world.GetBlockPlayerLooksAt(rayStart, rayDir);
}

void Game::WorldUpdateLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplGlfw_Sleep(52);
        world.Update(camera.Position.x, camera.Position.z);
    }
}

void Game::ProcessInput()
{
    glm::vec2 mouseOffset = InputManager::GetMouseDelta();
    if (InputManager::IsKeyDown(GLFW_KEY_W))
        camera.ProcessKeyboard(FORWARD, m_state.deltaTime);
    if (InputManager::IsKeyDown(GLFW_KEY_S))
        camera.ProcessKeyboard(BACKWARD, m_state.deltaTime);
    if (InputManager::IsKeyDown(GLFW_KEY_A))
        camera.ProcessKeyboard(LEFT, m_state.deltaTime);
    if (InputManager::IsKeyDown(GLFW_KEY_D))
        camera.ProcessKeyboard(RIGHT, m_state.deltaTime);
    if (InputManager::IsKeyDown(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);

    InputManager::UpdateCursorLock();

    if (InputManager::lockCursor)
    {
        camera.ProcessMouseMovement(mouseOffset.x, mouseOffset.y);
        if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT))
        {
            if (m_state.selectedBlock.blockInfo)
            {
                world.SetBlock(m_state.lastRCR.facePos.x,
                    m_state.lastRCR.facePos.y,
                    m_state.lastRCR.facePos.z, m_state.selectedBlock);
            }
        }
        else
        {
            if (InputManager::IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT))
            {
                if (m_state.lastRCR.blockInfo)
                {
                    auto block = Block(BlocksManager::GetBlockInfoByName("air"));
                    world.SetBlock(m_state.lastRCR.blockPos.x, m_state.lastRCR.blockPos.y, m_state.lastRCR.blockPos.z, block);
                }
            }
        }
    }

    InputManager::Update();
}

void Game::DisplayFrame(std::vector<std::vector<uint8_t>> & data)
{
    world.DisplayFrame(data);
}

void Game::Cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}
