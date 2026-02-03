#pragma once

#include <vector>
#include "Mesh.h"
#include "Shader.h"
#include <memory>
#include <GL/glew.h>

class Renderer
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

public:
    std::unique_ptr<Shader> defaultShader;
    std::unique_ptr<Shader> transparentShader;
    std::unique_ptr<Shader> hilightShader;
    std::unique_ptr<Shader> zPrepassShader;
    std::unique_ptr<Shader> geometryShader;
    std::unique_ptr<Shader> lightingShader;

    GBuffer gBuffer;
    ZBuffer zBuffer;

public:
    Renderer();
    ~Renderer();
    void Initialise(int width, int height);
    bool InitShaders();
    void InitGBuffer();
    void InitZBuffer();
    void AddToBatch(Mesh* mesh);
    void ClearBarchQueue() { m_batchQueue.clear(); }

    void ZPrepass(const glm::mat4& view, const glm::mat4& projection);
    void GeometryPass(const glm::mat4& view, const glm::mat4& projection);
    void GeometryPassNoPrepass(const glm::mat4& view, const glm::mat4& projection);
    void LightingPass(const glm::vec3& viewPos) const;
    void CopyDepthFromGBufferToQuad() const;
    void RenderQuad() const;

    void RenderMeshes();

    void ActivateTexture() const;

    void CopyDepthFromZBufferToGBuffer() const;
    void SetZPrepassState() const;
    void SetGeometryPassState(bool usePrepass) const;
    void SetLightingPassState();
    void ResetRenderState();
    void SetupLightingPassTextures() const;

private:
    void InitTexture();


private:
    GLuint m_blocksTextureAtlas = 0;
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;
    std::vector<Mesh*> m_batchQueue;
    int m_width = 0, m_height = 0;
};