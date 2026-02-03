#include "Game.h"

#ifdef BOJE_CHEL
#include <windows.h>
#include <psapi.h>
#endif 

#include <renderer/Frustum.h>
#include <renderer/Shader.h>
#include <videoWorld/chunk/rendering/RayCaster.h>
#include <memory>
#include "Camera.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <videoWorld/block/Block.h>
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <renderer/lighting/Lights.h>

void Game::RenderWorld()
{
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective
    (
        glm::radians(camera.Zoom),
        static_cast<GLfloat>(m_state.windowWidth) /
        static_cast<GLfloat>(m_state.windowHeight),
        0.1f, 1500.0f
    );
 

    {
        {
            auto& blocksGraphicalData = world.GetBlocksGraphicalData();
            auto& solidGeometry = blocksGraphicalData[static_cast<int>(GeometryType::Solid)];
            
            solidGeometry.LoadGraphicalData();

            for (auto& bufferItem : solidGeometry)
            {
                auto [min, max] = bufferItem.aabb;

                if (frustum.isAABBVisible(min, max))
                    AddToBatch(&bufferItem.mesh);
            }
        }

        glClearColor(0.33f, 0.66f, 1.0f, 1.0f);
        if (m_state.useZPrepass)
        {
            SetZPrepassState();

            zPrepassShader->Use();
            zPrepassShader->SetUniformMatrix4fv("view", view);
            zPrepassShader->SetUniformMatrix4fv("projection", projection);
            zPrepassShader->SetUniformMatrix4fv("model", glm::mat4(1.0f));

            RenderMeshes();

            ResetRenderState();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            CopyDepthFromZBufferToGBuffer();
        }

        SetGeometryPassState(m_state.useZPrepass);

        geometryShader->Use();
        geometryShader->SetUniformMatrix4fv("model", glm::mat4(1.0f));
        geometryShader->SetUniformMatrix4fv("view", view);
        geometryShader->SetUniformMatrix4fv("projection", projection);
        geometryShader->SetUniform1i("blocksTexture", 5);

        RenderMeshes();

        ResetRenderState();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.52f, 0.8f, 0.96f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        lightingShader->Use();
        lightingShader->SetUniform3f("viewPos", camera.Position);

        lightingShader->SetUniform3f("sunLight.direction",
            glm::vec3(m_state.sunLight.direction));
        lightingShader->SetUniform3f("sunLight.ambient",
            glm::vec3(m_state.sunLight.ambient));
        lightingShader->SetUniform3f("sunLight.diffuse",
            glm::vec3(m_state.sunLight.diffuse));
        lightingShader->SetUniform3f("sunLight.specular",
            glm::vec3(m_state.sunLight.specular));

        lightingShader->SetUniform3f("tempPlayerLight.position",
            glm::vec3(camera.Position));
        lightingShader->SetUniform3f("tempPlayerLight.ambient",
            glm::vec3(m_state.tempPlayerLight.ambient));
        lightingShader->SetUniform3f("tempPlayerLight.diffuse",
            glm::vec3(m_state.tempPlayerLight.diffuse));
        lightingShader->SetUniform3f("tempPlayerLight.specular",
            glm::vec3(m_state.tempPlayerLight.specular));
        lightingShader->SetUniform1f("tempPlayerLight.constant",
            m_state.tempPlayerLight.constant);
        lightingShader->SetUniform1f("tempPlayerLight.linear",
            m_state.tempPlayerLight.linear);
        lightingShader->SetUniform1f("tempPlayerLight.quadratic",
            m_state.tempPlayerLight.quadratic);

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

        ClearBatchQueue();
        
        {
            auto& blocksGraphicalData = world.GetBlocksGraphicalData();
            auto& transparentGeometry = blocksGraphicalData[static_cast<int>(GeometryType::Transparent)];
            transparentGeometry.LoadGraphicalData();
            for (auto& bufferItem : transparentGeometry)
            {
                auto [min, max] = bufferItem.aabb;
                if (frustum.isAABBVisible(min, max))
                    AddToBatch(&bufferItem.mesh);
            }
        }

        CopyDepthFromGBufferToDefault();
        defaultShader->Use();
        defaultShader->SetUniformMatrix4fv("model", glm::mat4(1.0f));
        defaultShader->SetUniformMatrix4fv("view", view);
        defaultShader->SetUniformMatrix4fv("projection", projection);
        defaultShader->SetUniform1i("blocksTexture", 5);
        defaultShader->SetUniform1i("useWindFX", 1);
        defaultShader->SetUniform1f("time", static_cast<float>(glfwGetTime()));
        defaultShader->SetUniform1f("windSpeed", m_state.windSpeed);
        defaultShader->SetUniform3f("viewPos", camera.Position);

        defaultShader->SetUniform3f("sunLight.direction",
            glm::vec3(m_state.sunLight.direction));
        defaultShader->SetUniform3f("sunLight.ambient",
            glm::vec3(m_state.sunLight.ambient));
        defaultShader->SetUniform3f("sunLight.diffuse",
            glm::vec3(m_state.sunLight.diffuse));
        defaultShader->SetUniform3f("sunLight.specular",
            glm::vec3(m_state.sunLight.specular));

        defaultShader->SetUniform3f("tempPlayerLight.position",
            glm::vec3(camera.Position));
        defaultShader->SetUniform3f("tempPlayerLight.ambient",
            glm::vec3(m_state.tempPlayerLight.ambient));
        defaultShader->SetUniform3f("tempPlayerLight.diffuse",
            glm::vec3(m_state.tempPlayerLight.diffuse));
        defaultShader->SetUniform3f("tempPlayerLight.specular",
            glm::vec3(m_state.tempPlayerLight.specular));
        defaultShader->SetUniform1f("tempPlayerLight.constant",
            m_state.tempPlayerLight.constant);
        defaultShader->SetUniform1f("tempPlayerLight.linear",
            m_state.tempPlayerLight.linear);
        defaultShader->SetUniform1f("tempPlayerLight.quadratic",
            m_state.tempPlayerLight.quadratic);

        ActivateTexture();
        RenderMeshes();

        ClearBatchQueue();
    }

    //m_state.lastRCR = PerformRayCast();
    //RenderHighlightCube(m_state.lastRCR);
}

void Game::RenderLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float thickness) const
{
    static GLuint VAO = 0, VBO = 0;
    glm::vec3 vertices[] = { start, end };

    if (VAO == 0)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    hilightShader->Use();

    glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom),
        static_cast<GLfloat>(m_state.windowWidth) / static_cast<GLfloat>(m_state.windowHeight),
        0.1f,
        1500.0f
    );
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    hilightShader->SetUniformMatrix4fv("projection", projection);
    hilightShader->SetUniformMatrix4fv("view", view);
    hilightShader->SetUniformMatrix4fv("model", model);
    hilightShader->SetUniform4f("color", color);

    glLineWidth(thickness);

    glEnable(GL_LINE_SMOOTH);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void Game::RenderHighlightCube(const RayCastResult& rcr) const
{
    if (!rcr.hit) return;

    static bool initialized = false;
    static GLuint VAOs[6] = { 0 };
    static GLuint VBOs[6] = { 0 };

    static const glm::vec3 faceVertices[6][6] = {
        {
            {-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
            {0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}
        },
        {
            {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}
        },
        {
            {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f},
            {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}
        },
        {
            {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}
        },
        {
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f},
            {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}
        },
        {
            {-0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f},
            {0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}
        }
    };

    static const glm::vec3 normals[6] = {
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    };

    if (!initialized)
    {
        for (int i = 0; i < 6; i++)
        {
            glGenVertexArrays(1, &VAOs[i]);
            glGenBuffers(1, &VBOs[i]);

            glBindVertexArray(VAOs[i]);
            glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(faceVertices[i]), faceVertices[i], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);
        }
        initialized = true;
    }

    int faceIndex = -1;
    for (int i = 0; i < 6; i++)
    {
        if (glm::all(glm::epsilonEqual(rcr.normal, normals[i], 0.001f)))
        {
            faceIndex = i;
            break;
        }
    }

    if (faceIndex == -1) return;

    glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom),
        static_cast<GLfloat>(m_state.windowWidth) / static_cast<GLfloat>(m_state.windowHeight),
        0.1f,
        1500.0f
    );
    glm::mat4 view = camera.GetViewMatrix();

    glm::vec3 offset = rcr.normal * 0.01f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(rcr.blockPos) + offset);

    hilightShader->Use();
    hilightShader->SetUniformMatrix4fv("projection", projection);
    hilightShader->SetUniformMatrix4fv("view", view);
    hilightShader->SetUniformMatrix4fv("model", model);

    hilightShader->SetUniform4f("color", glm::vec4(1.0f, 1.0f, 1.0f, 0.2f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(VAOs[faceIndex]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Game::RenderCrosshair() const
{
    static GLuint VAO = 0, VBO = 0;
    static glm::vec2 vertices[] =
    {
        {-0.02f, 0.0f}, {0.02f, 0.0f},
        {0.0f, -0.025f}, {0.0f, 0.025f}
    };

    if (VAO == 0)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    hilightShader->Use();

    static glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    static glm::mat4 view = glm::mat4(1.0f);
    static glm::mat4 model = glm::mat4(1.0f);

    hilightShader->SetUniformMatrix4fv("projection", projection);
    hilightShader->SetUniformMatrix4fv("view", view);
    hilightShader->SetUniformMatrix4fv("model", model);
    hilightShader->SetUniform4f("color", glm::vec4(1.0f, 1.0f, 1.0f, 0.9f));

    glDisable(GL_DEPTH_TEST);

    glLineWidth(2.0f);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void Game::SetZPrepassState() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, zBuffer.FBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDisable(GL_BLEND);
}

void Game::SetGeometryPassState(bool usePrepass) const
{
    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.FBO);
    glClearColor(0.33f, 0.66f, 1.0f, 1.0f);

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

void Game::ResetRenderState()
{
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void Game::CopyDepthFromZBufferToGBuffer() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, zBuffer.FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer.FBO);
    glBlitFramebuffer(0, 0, m_state.windowWidth, m_state.windowHeight, 0, 0,
        m_state.windowWidth, m_state.windowHeight,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::CopyDepthFromGBufferToDefault() const
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.FBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, m_state.windowWidth, m_state.windowHeight, 0, 0,
        m_state.windowWidth, m_state.windowHeight,
        GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Game::ActivateTexture() const
{
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, m_blocksTextureAtlas);
}

void Game::RenderQuad() const
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Game::RenderMeshes()
{
    ActivateTexture();

    for (auto mesh : m_batchQueue)
    {
        mesh->Draw();
    }
    //std::cout << m_batchQueue.size() << '\n';

    glBindTexture(GL_TEXTURE_2D, 0);
}