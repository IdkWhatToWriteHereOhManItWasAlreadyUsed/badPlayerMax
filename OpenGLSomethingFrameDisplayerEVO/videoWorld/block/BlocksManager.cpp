#include "BlocksManager.h"
#include <iostream>

using json = nlohmann::json;

std::vector<std::unique_ptr<BlockInfo>> BlocksManager::m_blocks;
std::unordered_map<std::string, BlockInfo*> BlocksManager::m_nameToBlock;
std::unordered_map<std::string, Geometry> BlocksManager::m_geometries;

bool BlocksManager::LoadBlocksData(const std::string& blocksJsonPath, const std::string& geometriesJsonPath)
{
    if (!LoadGeometries(geometriesJsonPath))
    {
        std::cout << "Cannot load geometries" << std::endl;
        return false;
    }

    if (!LoadBlocks(blocksJsonPath))
    {
        std::cout << "Cannot load blocks" << std::endl;
        return false;
    }

    return true;
}

bool BlocksManager::LoadBlocks(const std::string& filePath)
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        json data = json::parse(file);

        if (data.contains("blocks") && data["blocks"].is_array())
        {
            for (const auto& blockJson : data["blocks"]) {
                auto blockType = std::make_unique<BlockInfo>(ParseBlockType(blockJson));

                if (m_geometries.find(blockType->geometry_type) == m_geometries.end())
                    continue;

                BlockInfo* blockPtr = blockType.get();
                m_nameToBlock[blockType->block_name] = blockPtr;
                m_blocks.push_back(std::move(blockType));
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool BlocksManager::LoadGeometries(const std::string& filePath)
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        json data = json::parse(file);

        if (data.contains("geometries") && data["geometries"].is_array())
            for (const auto& geometryJson : data["geometries"])
            {
                Geometry geometry = ParseGeometry(geometryJson);
                m_geometries[geometry.geometry_type] = geometry;
            }

        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

BlockInfo BlocksManager::ParseBlockType(const nlohmann::json& blockJson)
{
    BlockInfo blockInfo;

    blockInfo.block_name = blockJson["block_name"];
    blockInfo.geometry_type = blockJson["geometry_type"];

    if (blockJson.contains("is_transparent")) {
        blockInfo.is_transparent = blockJson["is_transparent"];
    }
    else {
        blockInfo.is_transparent = false;
    }

    if (blockJson.contains("is_solid")) {
        blockInfo.is_solid = blockJson["is_solid"];
    }
    else {
        blockInfo.is_solid = false;
    }

    if (blockJson.contains("faces") && blockJson["faces"].is_array())
    {
        for (const auto& faceJson : blockJson["faces"])
        {
            BlockFaceTexture face;
            face.face_name = faceJson["face_name"];
            face.texture_x = faceJson["texture_x"];
            face.texture_z = faceJson["texture_z"];
            blockInfo.faces.push_back(face);
        }
    }

    return blockInfo;
}

Geometry BlocksManager::ParseGeometry(const nlohmann::json& geometryJson)
{
    Geometry geometry;

    geometry.geometry_type = geometryJson["geometry_type"];

    // is_solid удален - теперь используем is_transparent из блока

    if (geometryJson.contains("faces") && geometryJson["faces"].is_array())
    {
        for (const auto& faceJson : geometryJson["faces"])
            geometry.faces.push_back(ParseFace(faceJson));
    }

    if (geometryJson.contains("can_cull_faces") && geometryJson["can_cull_faces"].is_array())
    {
        for (const auto& faceName : geometryJson["can_cull_faces"])
            geometry.can_cull_faces.push_back(faceName);
    }

    return geometry;
}

Face BlocksManager::ParseFace(const nlohmann::json& faceJson)
{
    Face face;

    face.name = faceJson["name"];

    // парсинг нормали
    if (faceJson.contains("normal") && faceJson["normal"].is_array())
    {
        const auto& normalArray = faceJson["normal"];
        face.normal = glm::vec3(normalArray[0], normalArray[1], normalArray[2]);
    }
    else
    {
        // Автоматическое вычисление нормали если не указана в JSON
        face.normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    if (faceJson.contains("vertices") && faceJson["vertices"].is_array())
    {
        for (const auto& vertexJson : faceJson["vertices"])
        {
            face.vertices.push_back(ParseVertex(vertexJson));
            face.vertices[face.vertices.size() - 1].Normal = face.normal;
        }
    }

    if (faceJson.contains("indices") && faceJson["indices"].is_array())
    {
        for (const auto& index : faceJson["indices"])
            face.indices.push_back(index);
    }

    return face;
}

Vertex BlocksManager::ParseVertex(const nlohmann::json& vertexJson)
{
    Vertex vertex;

    if (vertexJson.contains("position") && vertexJson["position"].is_array())
    {
        const auto& posArray = vertexJson["position"];
        vertex.Position = glm::vec3(posArray[0], posArray[1], posArray[2]);
    }

    if (vertexJson.contains("tex_coords") && vertexJson["tex_coords"].is_array())
    {
        const auto& texArray = vertexJson["tex_coords"];
        vertex.TexCoords = glm::vec2(texArray[0], texArray[1]);
    }

    return vertex;
}

const BlockInfo* BlocksManager::GetBlockInfoByName(const std::string& name)
{
    auto it = m_nameToBlock.find(name);
    return it != m_nameToBlock.end() ? it->second : nullptr;
}

const Geometry* BlocksManager::GetGeometry(const std::string& geometryType)
{
    auto it = m_geometries.find(geometryType);
    return it != m_geometries.end() ? &it->second : nullptr;
}

const Geometry* BlocksManager::GetGeometryForBlock(const std::string& blockName)
{
    const BlockInfo* blockType = GetBlockInfoByName(blockName);
    return blockType ? GetGeometry(blockType->geometry_type) : nullptr;
}

const std::vector<std::unique_ptr<BlockInfo>>& BlocksManager::GetAllBlockTypes()
{
    return m_blocks;
}

glm::vec3 BlocksManager::GetFaceNormal(const std::string& geometryType, const std::string& faceName)
{
    const Geometry* geometry = GetGeometry(geometryType);
    if (!geometry) return glm::vec3(0.0f);

    for (const auto& face : geometry->faces) {
        if (face.name == faceName) {
            return face.normal;
        }
    }

    return glm::vec3(0.0f);
}