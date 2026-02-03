#pragma once

#include <vector>
#include <string>
#include <renderer/Mesh.h>

struct Face
{
    std::string name;
    glm::vec3 normal;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

struct Geometry
{
    std::string geometry_type;
    std::vector<Face> faces;
    std::vector<std::string> can_cull_faces;
};

struct BlockFaceTexture
{
    std::string face_name;
    int texture_x;
    int texture_z;
};

struct BlockInfo
{
    std::string block_name;
    std::string geometry_type;
    std::vector<BlockFaceTexture> faces;
    bool is_solid;
    bool is_transparent;
};

struct Block
{
    const BlockInfo* blockInfo = nullptr;
    Block() { blockInfo = nullptr; };
    Block(const BlockInfo* info) : blockInfo(info)
    {
    }

    std::string getName() const
    {
        return blockInfo->block_name;
    }
};