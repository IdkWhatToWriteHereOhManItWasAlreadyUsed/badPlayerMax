#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <fstream>
#include "renderer/Mesh.h"
#include <libs/nlohmann/json.hpp>
#include "Block.h"

class BlocksManager
{
private:
    static std::vector<std::unique_ptr<BlockInfo>> m_blocks;
    static std::unordered_map<std::string, BlockInfo*> m_nameToBlock;
    static std::unordered_map<std::string, Geometry> m_geometries;

public:
    BlocksManager() = default;
    ~BlocksManager() = default;

    static bool LoadBlocksData(const std::string& blocksJsonPath, const std::string& geometriesJsonPath);

    static const BlockInfo* GetBlockInfoByName(const std::string& name);
    static const std::vector<std::unique_ptr<BlockInfo>>& GetAllBlockTypes();
    static const std::unordered_map<std::string, Geometry>& GetAllGeometries() { return m_geometries; }

    static const Geometry* GetGeometry(const std::string& geometryType);
    static const Geometry* GetGeometryForBlock(const std::string& blockName);

    static size_t GetBlocksCount() { return m_blocks.size(); }
    static size_t GetGeometriesCount() { return m_geometries.size(); }

    static glm::vec3 GetFaceNormal(const std::string& geometryType, const std::string& faceName);

private:
    static bool LoadBlocks(const std::string& filePath);
    static bool LoadGeometries(const std::string& filePath);

    static Vertex ParseVertex(const nlohmann::json& vertexJson);
    static Face ParseFace(const nlohmann::json& faceJson);
    static Geometry ParseGeometry(const nlohmann::json& geometryJson);
    static BlockInfo ParseBlockType(const nlohmann::json& blockJson);
};