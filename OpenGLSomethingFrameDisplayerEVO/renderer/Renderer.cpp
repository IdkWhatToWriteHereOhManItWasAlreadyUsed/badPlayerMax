#include "Renderer.h"
#include <iostream>
#include <ostream>
#include <GL/glew.h>
#include <glm/fwd.hpp>
#include "Mesh.h"
#include "Shader.h"
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <memory>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <libs/stb_image.h>

Renderer::Renderer()
{
    m_batchQueue.reserve(5000);
}

Renderer::~Renderer()
{
}

void Renderer::Initialise(int width, int height)
{
    m_width = width;
    m_height = height;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSwapInterval(0);

    InitTexture();
    InitShaders();

    InitZBuffer();
    InitGBuffer();

    float quadVertices[] =
    {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer::InitTexture()
{
    int width, height, channels;

    unsigned char* image = stbi_load("res/textures/blocks.png", &width, &height, &channels, STBI_rgb_alpha);

    if (image == nullptr) {
        std::cerr << "Failed to load texture: " << "res/textures/blocks.png" << std::endl;
        std::cerr << "STBI error: " << stbi_failure_reason() << std::endl;
        return;
    }

    std::cout << "Loaded texture: " << "res/textures/blocks.png"
        << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Если каналов 4 (RGBA), можешь установить CLAMP_TO_EDGE
    if (channels == 4) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Мы просим stbi_load загрузить в RGBA, так что format = GL_RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_blocksTextureAtlas = textureID;
}

bool Renderer::InitShaders()
{
    try
    {
        defaultShader = std::make_unique<Shader>("res/shaders/default.vs.glsl", "res/shaders/default.frag.glsl");
        hilightShader = std::make_unique<Shader>("res/shaders/hilight.vs.glsl", "res/shaders/hilight.frag.glsl");
        zPrepassShader = std::make_unique<Shader>("res/shaders/zPrepass.vs.glsl", "res/shaders/zPrepass.frag.glsl");
        geometryShader = std::make_unique<Shader>("res/shaders/gPass.vs.glsl", "res/shaders/gPass.frag.glsl");
        lightingShader = std::make_unique<Shader>("res/shaders/lightPass.vs.glsl", "res/shaders/lightPass.frag.glsl");

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout << "Failed to initialize shaders: " << e.what() << std::endl;
        return false;
    }
}

void Renderer::InitGBuffer()
{
    glGenFramebuffers(1, &gBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);

    glGenTextures(static_cast<int>(GBufferTexture::Count), gBuffer.textures);

    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Positions)]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        gBuffer.textures[static_cast<int>(GBufferTexture::Positions)], 0);

    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Normal)]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
        gBuffer.textures[static_cast<int>(GBufferTexture::Normal)], 0);

    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::AlbedoSpec)]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
        gBuffer.textures[static_cast<int>(GBufferTexture::AlbedoSpec)], 0);

    glGenTextures(1, &gBuffer.depthTexture);
    glBindTexture(GL_TEXTURE_2D, gBuffer.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
        gBuffer.depthTexture, 0);

    GLuint attachments[3] =
    {
        GL_COLOR_ATTACHMENT0,  // Positions
        GL_COLOR_ATTACHMENT1,  // Normal
        GL_COLOR_ATTACHMENT2   // AlbedoSpec
    };
    glDrawBuffers(3, attachments);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "G-Buffer FBO error: " << status << std::endl;
        switch (status) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            std::cout << "Incomplete attachment" << std::endl; break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            std::cout << "Missing attachment" << std::endl; break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            std::cout << "Unsupported format" << std::endl; break;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::InitZBuffer()
{
    glGenFramebuffers(1, &zBuffer.FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, zBuffer.FBO);

    glGenTextures(1, &zBuffer.depthTexture);
    glBindTexture(GL_TEXTURE_2D, zBuffer.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
        zBuffer.depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Can not initialise z-prepass" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CopyDepthFromZBufferToGBuffer() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, zBuffer.FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer.FBO);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ActivateTexture() const
{
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, m_blocksTextureAtlas);
}

void Renderer::AddToBatch(Mesh* mesh)
{
    m_batchQueue.push_back(mesh);
}

void Renderer::SetZPrepassState() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, zBuffer.FBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);
}

void Renderer::SetGeometryPassState(bool usePrepass) const
{
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    if (usePrepass)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthFunc(GL_EQUAL);
        glDepthMask(GL_FALSE);
    }
    else
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
    }
}

void Renderer::SetLightingPassState()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.52f, 0.8f, 0.96f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::ResetRenderState()
{
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Renderer::ZPrepass(const glm::mat4& view, const glm::mat4& projection)
{
    SetZPrepassState();

    zPrepassShader->Use();
    zPrepassShader->SetUniformMatrix4fv("view", view);
    zPrepassShader->SetUniformMatrix4fv("projection", projection);
    zPrepassShader->SetUniformMatrix4fv("model", glm::mat4(1.0f));

    for (auto* mesh : m_batchQueue)
    {
        mesh->Draw();
    }

    ResetRenderState();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    CopyDepthFromZBufferToGBuffer();
}

void Renderer::GeometryPass(const glm::mat4& view, const glm::mat4& projection)
{
    SetGeometryPassState(true);

    geometryShader->Use();
    geometryShader->SetUniformMatrix4fv("model", glm::mat4(1.0f));
    geometryShader->SetUniformMatrix4fv("view", view);
    geometryShader->SetUniformMatrix4fv("projection", projection);
    geometryShader->SetUniform1i("blocksTexture", 5);

    ActivateTexture();

    for (auto* mesh : m_batchQueue)
    {
        mesh->Draw();
    }

    ResetRenderState();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_batchQueue.clear();
}

void Renderer::GeometryPassNoPrepass(const glm::mat4& view, const glm::mat4& projection)
{
    SetGeometryPassState(false);

    geometryShader->Use();
    geometryShader->SetUniformMatrix4fv("model", glm::mat4(1.0f));
    geometryShader->SetUniformMatrix4fv("view", view);
    geometryShader->SetUniformMatrix4fv("projection", projection);
    geometryShader->SetUniform1i("blocksTexture", 5);

    ActivateTexture();

    for (auto* mesh : m_batchQueue)
    {
        mesh->Draw();
    }

    ResetRenderState();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::LightingPass(const glm::vec3& viewPos) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.52f, 0.8f, 0.96f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    lightingShader->Use();
    lightingShader->SetUniform3f("viewPos", viewPos);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Positions)]);
    lightingShader->SetUniform1i("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Normal)]);
    lightingShader->SetUniform1i("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::AlbedoSpec)]);
    lightingShader->SetUniform1i("gAlbedoSpec", 2);

    RenderQuad();

    glEnable(GL_DEPTH_TEST);
}

void Renderer::SetupLightingPassTextures() const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Positions)]);
    lightingShader->SetUniform1i("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::Normal)]);
    lightingShader->SetUniform1i("gNormal", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.textures[static_cast<int>(GBufferTexture::AlbedoSpec)]);
    lightingShader->SetUniform1i("gAlbedoSpec", 2);
}


void Renderer::CopyDepthFromGBufferToQuad() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderQuad() const
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::RenderMeshes()
{
    ActivateTexture();

    for (auto mesh : m_batchQueue)
    {
        mesh->Draw();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}