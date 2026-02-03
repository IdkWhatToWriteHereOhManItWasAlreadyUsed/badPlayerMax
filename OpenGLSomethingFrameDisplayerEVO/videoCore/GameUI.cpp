#include "Game.h"

#ifdef BOJE_CHEL
#include <windows.h>
#include <psapi.h>
#endif 

#include <renderer/Frustum.h>
#include <videoWorld/block/BlockMeshGenerator.h>
#include <renderer/Shader.h>
#include <videoWorld/chunk/generation/ChunkGenerator.h>
#include <videoWorld/chunk/rendering/RayCaster.h>
#include <videoWorld/block/BlocksManager.h>
#include <imgui/imgui.h>
#include <memory>
#include <cmath>
#include "Camera.h"
#include <imgui/imgui_impl_glfw.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/fwd.hpp>
#include <videoWorld/block/Block.h>
#include <videoWorld/chunk/rendering/ChunkGraphicalData.h>
#include <renderer/lighting/Lights.h>

static void DirectionalLightEditor(DirectionalLight& sunLight)
{
    if (ImGui::CollapsingHeader("Sun Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static float timeOfDay = 12.0f;
        ImGui::SliderFloat("Time of Day", &timeOfDay, 0.0f, 24.0f, "%.1f h");

        float t = timeOfDay / 24.0f;
        float angle = (t - 0.25f) * 2.0f * glm::pi<float>();
        float height = glm::sin(angle) * 0.85f;

        float horizontal = glm::sqrt(1.0f - height * height);
        sunLight.direction = glm::normalize(glm::vec3(
            glm::cos(angle) * horizontal,
            height,
            -glm::sin(angle) * horizontal
        ));

        glm::vec3 sunColor;

        if (timeOfDay >= 22.0f || timeOfDay < 4.0f) {
            float nightFactor = (timeOfDay >= 22.0f)
                ? (timeOfDay - 22.0f) / 2.0f
                : (timeOfDay + 2.0f) / 6.0f;
            nightFactor = glm::clamp(nightFactor, 0.0f, 1.0f);

            sunColor = glm::mix(
                glm::vec3(0.08f, 0.08f, 0.12f),
                glm::vec3(0.12f, 0.12f, 0.18f),
                glm::abs(nightFactor - 0.5f) * 2.0f
            );
        }
        else if (timeOfDay >= 4.0f && timeOfDay < 8.0f) {
            float dawnFactor = (timeOfDay - 4.0f) / 4.0f;
            sunColor = glm::mix(
                glm::vec3(0.12f, 0.12f, 0.18f),
                glm::vec3(0.7f, 0.5f, 0.25f),
                dawnFactor
            );
        }
        else if (timeOfDay >= 8.0f && timeOfDay < 16.0f) {
            float dayFactor = (timeOfDay - 8.0f) / 8.0f;
            float whiteness = 1.0f - glm::abs(dayFactor - 0.5f) * 2.0f;

            sunColor = glm::mix(
                glm::vec3(0.8f, 0.75f, 0.65f),
                glm::vec3(1.0f, 0.98f, 0.95f),
                whiteness
            );
        }
        else {
            float duskFactor = (timeOfDay - 16.0f) / 6.0f;
            sunColor = glm::mix(
                glm::vec3(1.0f, 0.98f, 0.95f),
                glm::vec3(0.75f, 0.45f, 0.2f),
                glm::min(duskFactor * 1.5f, 1.0f)
            );

            if (timeOfDay > 20.0f) {
                float toNight = (timeOfDay - 20.0f) / 2.0f;
                sunColor = glm::mix(
                    sunColor,
                    glm::vec3(0.12f, 0.12f, 0.18f),
                    toNight
                );
            }
        }

        float brightness = glm::clamp(height * 0.7f + 0.6f, 0.15f, 1.0f);

        sunLight.ambient = sunColor * (0.25f + 0.15f * brightness);

        sunLight.diffuse = sunColor * (0.4f + 0.3f * brightness);

        sunLight.specular = glm::vec3(0.35f, 0.35f, 0.4f) * brightness * 0.8f;

        ImGui::SeparatorText("Light Balance");

        static float ambientMult = 1.0f;
        static float diffuseMult = 0.8f;

        ImGui::SliderFloat("Ambient Boost", &ambientMult, 0.5f, 1.5f, "x%.2f");
        ImGui::SliderFloat("Diffuse Scale", &diffuseMult, 0.5f, 1.2f, "x%.2f");

        sunLight.ambient *= ambientMult;
        sunLight.diffuse *= diffuseMult;

        float maxDiffuse = 1.2f;
        sunLight.diffuse = glm::min(sunLight.diffuse, glm::vec3(maxDiffuse));

        ImGui::Separator();

        float contrastRatio = glm::length(sunLight.diffuse) / glm::length(sunLight.ambient);
        ImGui::Text("Contrast ratio: %.2f:1", contrastRatio);

        if (timeOfDay >= 22.0f || timeOfDay < 4.0f) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Night (brighter)");
        }

        ImGui::Separator();
        if (ImGui::Button("Soft Morning")) {
            timeOfDay = 7.0f;
            ambientMult = 1.1f;
            diffuseMult = 0.7f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Bright Noon")) {
            timeOfDay = 12.0f;
            ambientMult = 1.0f;
            diffuseMult = 0.9f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cozy Evening")) {
            timeOfDay = 19.0f;
            ambientMult = 1.2f;
            diffuseMult = 0.6f;
        }
    }
}

static void RenderPointLightControls(PointLight& pointLight)
{
    if (ImGui::CollapsingHeader("Player Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static glm::vec3 baseAmbient = glm::vec3(0.1f);
        static glm::vec3 baseDiffuse = glm::vec3(1.0f);
        static glm::vec3 baseSpecular = glm::vec3(1.0f);

        static float ambientIntensity = 1.0f;
        static float diffuseIntensity = 1.0f;
        static float specularIntensity = 1.0f;
        static float radius = 20.0f;

        ImGui::Separator();

        ImGui::SeparatorText("Intensity");

        ImGui::Text("Overall Brightness:");
        if (ImGui::SliderFloat("##OverallBrightness", &diffuseIntensity, 0.0f, 3.0f, "%.2f"))
        {
            pointLight.diffuse = baseDiffuse * diffuseIntensity;
        }

        ImGui::Text("Ambient:");
        if (ImGui::SliderFloat("##AmbientIntensity", &ambientIntensity, 0.0f, 2.0f, "%.2f"))
        {
            pointLight.ambient = baseAmbient * ambientIntensity;
        }

        ImGui::Text("Specular:");
        if (ImGui::SliderFloat("##SpecularIntensity", &specularIntensity, 0.0f, 2.0f, "%.2f"))
        {
            pointLight.specular = baseSpecular * specularIntensity;
        }

        ImGui::SeparatorText("Colors");

        ImGui::Text("Ambient Color");
        if (ImGui::ColorEdit3("##AmbientColor", &baseAmbient[0], ImGuiColorEditFlags_NoInputs))
        {
            pointLight.ambient = baseAmbient * ambientIntensity;
        }

        ImGui::Text("Diffuse Color");
        if (ImGui::ColorEdit3("##DiffuseColor", &baseDiffuse[0], ImGuiColorEditFlags_NoInputs))
        {
            pointLight.diffuse = baseDiffuse * diffuseIntensity;
        }

        ImGui::Text("Specular Color");
        if (ImGui::ColorEdit3("##SpecularColor", &baseSpecular[0], ImGuiColorEditFlags_NoInputs))
        {
            pointLight.specular = baseSpecular * specularIntensity;
        }

        ImGui::SeparatorText("Distance & Falloff");

        ImGui::Text("Light Radius:");
        if (ImGui::SliderFloat("##Radius", &radius, 5.0f, 100.0f, "%.1f units"))
        {
            pointLight.linear = 2.0f / radius;
            pointLight.quadratic = 1.0f / (radius * radius);
        }

        if (ImGui::TreeNode("Advanced Falloff Settings"))
        {
            ImGui::Text("Constant: %.2f", pointLight.constant);
            ImGui::Text("Linear: %.4f", pointLight.linear);
            ImGui::Text("Quadratic: %.6f", pointLight.quadratic);

            ImGui::DragFloat("Linear##Manual", &pointLight.linear, 0.001f, 0.001f, 0.5f, "%.4f");
            ImGui::DragFloat("Quadratic##Manual", &pointLight.quadratic, 0.0001f, 0.0001f, 0.05f, "%.6f");

            ImGui::TreePop();
        }

        ImGui::SeparatorText("Presets");

        ImGui::Text("Quick presets:");

        if (ImGui::Button("Torch"))
        {
            baseDiffuse = glm::vec3(1.0f, 0.4f, 0.1f);
            baseAmbient = glm::vec3(0.3f, 0.12f, 0.03f);
            baseSpecular = glm::vec3(1.0f, 0.5f, 0.2f);
            diffuseIntensity = 1.5f;
            radius = 15.0f;

            pointLight.diffuse = baseDiffuse * diffuseIntensity;
            pointLight.ambient = baseAmbient * ambientIntensity;
            pointLight.specular = baseSpecular * specularIntensity;
            pointLight.linear = 2.0f / radius;
            pointLight.quadratic = 1.0f / (radius * radius);
        }

        ImGui::SameLine();

        if (ImGui::Button("Magic"))
        {
            baseDiffuse = glm::vec3(0.2f, 0.6f, 1.0f);
            baseAmbient = glm::vec3(0.06f, 0.18f, 0.3f);
            baseSpecular = glm::vec3(0.4f, 0.8f, 1.0f);
            diffuseIntensity = 1.2f;
            radius = 25.0f;

            pointLight.diffuse = baseDiffuse * diffuseIntensity;
            pointLight.ambient = baseAmbient * ambientIntensity;
            pointLight.specular = baseSpecular * specularIntensity;
            pointLight.linear = 2.0f / radius;
            pointLight.quadratic = 1.0f / (radius * radius);
        }

        ImGui::SameLine();

        if (ImGui::Button("Bright"))
        {
            baseDiffuse = glm::vec3(1.0f, 1.0f, 0.9f);
            baseAmbient = glm::vec3(0.4f, 0.4f, 0.35f);
            baseSpecular = glm::vec3(1.0f, 1.0f, 0.95f);
            diffuseIntensity = 2.0f;
            radius = 30.0f;

            pointLight.diffuse = baseDiffuse * diffuseIntensity;
            pointLight.ambient = baseAmbient * ambientIntensity;
            pointLight.specular = baseSpecular * specularIntensity;
            pointLight.linear = 2.0f / radius;
            pointLight.quadratic = 1.0f / (radius * radius);
        }

        ImGui::SameLine();

        if (ImGui::Button("Dim"))
        {
            baseDiffuse = glm::vec3(0.8f, 0.8f, 0.8f);
            baseAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
            baseSpecular = glm::vec3(0.5f, 0.5f, 0.5f);
            diffuseIntensity = 0.7f;
            radius = 10.0f;

            pointLight.diffuse = baseDiffuse * diffuseIntensity;
            pointLight.ambient = baseAmbient * ambientIntensity;
            pointLight.specular = baseSpecular * specularIntensity;
            pointLight.linear = 2.0f / radius;
            pointLight.quadratic = 1.0f / (radius * radius);
        }

        ImGui::Separator();

        if (ImGui::Button("Reset to Default"))
        {
            pointLight.position = glm::vec3(0.0f, 1.0f, 0.0f);
            baseAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
            baseDiffuse = glm::vec3(1.0f, 0.9f, 0.7f);
            baseSpecular = glm::vec3(1.0f, 1.0f, 0.9f);

            ambientIntensity = 1.0f;
            diffuseIntensity = 1.0f;
            specularIntensity = 1.0f;
            radius = 20.0f;

            pointLight.ambient = baseAmbient * ambientIntensity;
            pointLight.diffuse = baseDiffuse * diffuseIntensity;
            pointLight.specular = baseSpecular * specularIntensity;
            pointLight.constant = 1.0f;
            pointLight.linear = 2.0f / radius;
            pointLight.quadratic = 1.0f / (radius * radius);
        }

        ImGui::SameLine();

        if (ImGui::Button("Toggle ON/OFF"))
        {
            static bool wasEnabled = false;
            if (wasEnabled)
            {
                static glm::vec3 savedAmbient = pointLight.ambient;
                static glm::vec3 savedDiffuse = pointLight.diffuse;
                static glm::vec3 savedSpecular = pointLight.specular;

                pointLight.ambient = glm::vec3(0.0f);
                pointLight.diffuse = glm::vec3(0.0f);
                pointLight.specular = glm::vec3(0.0f);
                wasEnabled = false;
            }
            else
            {
                pointLight.ambient = baseAmbient * ambientIntensity;
                pointLight.diffuse = baseDiffuse * diffuseIntensity;
                pointLight.specular = baseSpecular * specularIntensity;
                wasEnabled = true;
            }
        }
    }
}

void Game::RenderUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.Position.x, camera.Position.y, camera.Position.z);
    ImGui::Text("Yaw: %.2f", camera.Yaw);
    ImGui::Text("Pitch: %.2f", camera.Pitch);
    ImGui::Text("FPS: %.1f", m_state.displayedFPS);

#ifdef БОЖЕ_ЧЕЛ
    ImGui::Text("Memory: %.2f MB", m_state.memoryMB);
    ImGui::Text("Memory Change: %+f MB", m_state.memoryChangeMB);
#endif

    if (ImGui::SliderInt("Loading Distance", &m_state.loadingDistance, 1, 52))
    {
        world.SetLoadingDistance(m_state.loadingDistance);
    }

    ImGui::SliderFloat("Speed", &camera.MovementSpeed, 10.0f, 520.0f);
    ImGui::SliderFloat("windSpeed", &m_state.windSpeed, 0.0f, 40.0f);
    ImGui::SliderFloat("FOV", &camera.Zoom, 1.0f, 100.0f);
    ImGui::Checkbox("Z-Prepass", &m_state.useZPrepass);

    ImGui::Separator();
    if (ImGui::BeginCombo("Selected Block", m_state.selectedBlock.blockInfo ? m_state.selectedBlock.blockInfo->block_name.c_str() : "None"))
    {
        for (const auto& blockInfo : BlocksManager::GetAllBlockTypes())
        {
            bool isSelected = m_state.selectedBlock.blockInfo && m_state.selectedBlock.blockInfo->block_name == blockInfo->block_name;
            if (ImGui::Selectable(blockInfo->block_name.c_str(), isSelected))
                m_state.selectedBlock = Block(blockInfo.get());

            if (isSelected)
                ImGui::SetItemDefaultFocus();

        }
        ImGui::EndCombo();
    }

    DirectionalLightEditor(m_state.sunLight);
    RenderPointLightControls(m_state.tempPlayerLight);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}