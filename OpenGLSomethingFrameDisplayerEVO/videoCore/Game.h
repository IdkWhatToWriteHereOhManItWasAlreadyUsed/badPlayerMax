#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>

#include "Camera.h"
#include <renderer/Frustum.h>
#include <videoWorld/chunk/rendering/RayCaster.h>
#include <memory>
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include <videoWorld/block/Block.h>
#include <renderer/lighting/Lights.h>

#include <vector>
#include <renderer/Mesh.h>
#include <renderer/Shader.h>
#include <videoWorld/VideoWorld.h>

struct GlobalState
{
    int windowWidth = 800;
    int windowHeight = 600;

    GLfloat deltaTime = 0.0f;
    GLfloat lastFrame = 0.0f;

    // World
    int loadingDistance = 4;

    // Rendering
    bool useZPrepass = false;

    // Lighting
    DirectionalLight sunLight;
    
   PointLight tempPlayerLight = PointLight{
        glm::vec3(10000.0f, 0.0f, 0.0f),
        glm::vec3(0.2f, 0.2f, 0.2f),
        glm::vec3(1.0f, 0.9f, 0.7f),
        glm::vec3(1.0f, 1.0f, 0.9f),
        1.0f,
        0.09f,
        0.032f
    };
/*
    PointLight tempPlayerLight = PointLight{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        0.00f,
        0.00f
    };
*/
    // ExtraFX
    float windSpeed = 10.0f;

    // Performance
    float displayedFPS = 0.0f;
    float memoryMB = 0.0f;
    float memoryChangeMB = 0.0f;

    RayCastResult lastRCR;
    Block selectedBlock;
};

class Game
{
private:
    enum class GBufferTexture
    {
        Positions = 0,
        Normal,
        AlbedoSpec,
        Count
    };

    struct GBuffer
    {
        GLuint FBO;
        GLuint textures[static_cast<int>(GBufferTexture::Count)];
        GLuint depthTexture;
    };

    struct ZBuffer
    {
        GLuint FBO;
        GLuint depthTexture;
    };

private:
    GlobalState m_state;

    std::unique_ptr<Shader> defaultShader;
    std::unique_ptr<Shader> transparentShader;
    std::unique_ptr<Shader> hilightShader;
    std::unique_ptr<Shader> zPrepassShader;
    std::unique_ptr<Shader> geometryShader;
    std::unique_ptr<Shader> lightingShader;

    GBuffer gBuffer;
    ZBuffer zBuffer;

    GLuint m_blocksTextureAtlas = 0;
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    std::vector<Mesh*> m_batchQueue;

public:
    bool Initialise(int width, int height, int videoWidth, int videoHeight);
    void Run();
    void Cleanup();
    Game();

private:
    bool InitIMGUI();
    bool InitWindow();
    bool InitBlocksManager();
    bool InitShaders();
    void InitTexture();
    void InitGBuffer();
    void InitZBuffer();

    void MainLoop();
    void WorldUpdateLoop();

    void UpdateTiming();
    void UpdatePerformanceStats();
    void UpdateCameraMatrices();
    void ProcessInput();
    RayCastResult PerformRayCast() const;

    void RenderUI();
    void RenderWorld();
    void RenderSolidGeometry();
    void RenderTransparentGeometry();
    void SetupLightingPass();
    void SetupDefaultShaderForTransparent();

    void RenderLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float thickness) const;
    void RenderHighlightCube(const RayCastResult& rcr) const;
    void RenderCrosshair() const;
    void RenderQuad() const;

    void SetZPrepassState() const;
    void SetGeometryPassState(bool usePrepass) const;
    void ResetRenderState();
    void CopyDepthFromZBufferToGBuffer() const;
    void CopyDepthFromGBufferToDefault() const;
    void ActivateTexture() const;
    void ClearBatchQueue() { m_batchQueue.clear(); }
    void AddToBatch(Mesh* mesh) { m_batchQueue.push_back(mesh); }

    void RenderMeshes();
/////////////////////////////
public:
    void DisplayFrame(std::vector<std::vector<uint8_t>>& data);

private:
    Camera camera = Camera(glm::vec3(216.0f, 343.0f, 164.0f));
    VideoWorld world;
    GLFWwindow* window = nullptr;
    ImGuiIO io;
    Frustum frustum;
};
