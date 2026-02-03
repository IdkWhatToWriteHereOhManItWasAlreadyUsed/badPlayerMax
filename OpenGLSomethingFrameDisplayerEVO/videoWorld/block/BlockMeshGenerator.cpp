/*

    0   1
    3   2    


    надо так стремиться каждую грань везд
    порядок     // 0 3 1   1 3 2
    // 0 3 1   1 3 2
    cubeVertices =
    {
        // Back face (z = -0.5f) 
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}},        // TR
        {{-0.5f,  0.5f, -0.5f}, {0.0625f, 0.0f}},     // TL
        {{-0.5f, -0.5f, -0.5f}, {0.0625f, 0.0625f}},  // BL
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0625f}},     // BR

        // Front face (z = 0.5f) 
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},        // TL
        {{ 0.5f,  0.5f,  0.5f}, {0.0625f, 0.0f}},     // TR
        {{ 0.5f, -0.5f,  0.5f}, {0.0625f, 0.0625f}},  // BR
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0625f}},     // BL

        // Top face (y = 0.5f)
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}},        // BL (far left)
        {{ 0.5f,  0.5f, -0.5f}, {0.0625f, 0.0f}},     // BR (far right)
        {{ 0.5f,  0.5f,  0.5f}, {0.0625f, 0.0625f}},  // TR (near right)
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0625f}},     // TL (near left)

        // Bottom face (y = -0.5f)
        {{-0.5f,  -0.5f,  0.5f}, {0.0f, 0.0f}},       // TL
        {{ 0.5f,  -0.5f,  0.5f}, {0.0625f, 0.0f}},    // TR
        {{ 0.5f,  -0.5f, -0.5f}, {0.0625f, 0.0625f}}, // BR
        {{-0.5f,  -0.5f, -0.5f}, {0.0f, 0.0625f}},    // BL

        // Left face (x = -0.5f)
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}},        // TL
        {{-0.5f,  0.5f,  0.5f}, {0.0625f, 0.0f}},     // TR
        {{-0.5f, -0.5f,  0.5f}, {0.0625f, 0.0625f}},  // BR
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0625f}},     // BL

        // Right face (x = 0.5f)
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},        // TL
        {{ 0.5f,  0.5f, -0.5f}, {0.0625f, 0.0f}},     // TR
        {{ 0.5f, -0.5f, -0.5f}, {0.0625f, 0.0625f}},  // BR
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0625f}},     // BL

    };
*/

#include "BlockMeshGenerator.h"
#include "BlocksManager.h"
#include <array>
#include <renderer/Mesh.h>
#include <vector>
#include <utility>
#include "Block.h"
#include <GL/glew.h>

std::unordered_map<std::string, VertexDataPair> BlockMeshGenerator::m_cachedGeometryMeshes;

void BlockMeshGenerator::InitialiseCachesMeshes()
{
    auto& geometries = BlocksManager::GetAllGeometries();
    for (auto& geometry : geometries)
    {
        VertexDataPair geometryGraphicalData;
        auto lastIndexOffset = 0;
        for (auto& face : geometry.second.faces)
        {
            geometryGraphicalData.first.insert(geometryGraphicalData.first.end(), face.vertices.begin(), face.vertices.end());
            for (auto index : face.indices)
                geometryGraphicalData.second.push_back(index + lastIndexOffset);
            lastIndexOffset += face.vertices.size();
        }

        m_cachedGeometryMeshes[geometry.first] = geometryGraphicalData;
    }
}

std::pair<std::vector<Vertex>, std::vector<GLuint>> BlockMeshGenerator::GetBlockMeshData(
    const Block& block,
    const Block* back,
    const Block* front,
    const Block* left,
    const Block* right,
    const Block* bottom,
    const Block* top
)
{
    std::vector<Vertex> blockVertices;
    std::vector<GLuint> blockIndices;
    if (!block.blockInfo)
        return std::pair(blockVertices, blockIndices);

    auto geometry = BlocksManager::GetGeometryForBlock(block.getName());

    if (!geometry->can_cull_faces.size())
    {
        auto lastIndexOffset = 0;
        for (auto& face : geometry->faces)
        {
            int texture_x = 0;
            int texture_z = 0;

            for (auto& blockFace : block.blockInfo->faces)
            {
                if (blockFace.face_name == face.name)
                {
                    texture_x = blockFace.texture_x;
                    texture_z = blockFace.texture_z;
                }
            }

            for (Vertex vertex : face.vertices)
            {
                vertex.TexCoords.x += (texture_x * 0.0625f);
                vertex.TexCoords.y += (texture_z * 0.0625f);
                blockVertices.push_back(vertex);
            }

            for (auto index : face.indices)
                blockIndices.push_back(index + lastIndexOffset);
            lastIndexOffset += face.vertices.size();
        }

        return std::pair(blockVertices, blockIndices);
    }
    else
    {
        //auto& facesThatCanBeCulled = geometry->can_cull_faces;
        // 
        // TODO: fix this архитектурный просчёт
        std::array<bool, 6> shouldCullFace{ false };

        shouldCullFace[1] = front ? !front->blockInfo->is_transparent : false;
        shouldCullFace[0] = back ? !back->blockInfo->is_transparent : false;
        shouldCullFace[2] = top ? !top->blockInfo->is_transparent : false;
        shouldCullFace[3] = bottom ? !bottom->blockInfo->is_transparent : false;
        shouldCullFace[4] = left ? !left->blockInfo->is_transparent : false;
        shouldCullFace[5] = right ? !right->blockInfo->is_transparent : false;

        // first - face data index in blockIndices, second - face index in list of geometry faces
        std::vector<std::pair<int, int>> addedFacesIndicesInVertexArray;
 
        auto addedFaceIndexCounter = 0;

       // auto& geometryGraphicalData = m_cachedGeometryMeshes[geometry->geometry_type];

        // работает, но насрано и не оч производительно
        // надо потом когдато переделать
        auto lastIndexOffset = 0;
        for (int i = 0; i < 6; i++)
        {
            if (!shouldCullFace[i])
            {
                blockVertices.insert(blockVertices.end(), geometry->faces[i].vertices.begin(), geometry->faces[i].vertices.end());

                for (auto index : geometry->faces[i].indices)
                    blockIndices.push_back(index + lastIndexOffset);
                lastIndexOffset += geometry->faces[i].vertices.size();

                addedFacesIndicesInVertexArray.push_back( {geometry->faces[i].vertices.size() * addedFaceIndexCounter++, i} );
            }
        }
        // setup texture coords        
        for (auto addedFaceIndex : addedFacesIndicesInVertexArray)
        {
            int texture_x = 0;
            int texture_z = 0;
            auto& addedFace = block.blockInfo->faces[addedFaceIndex.second];
            texture_x = addedFace.texture_x;
            texture_z = addedFace.texture_z;

            for (auto j = 0; j < geometry->faces[addedFaceIndex.second].vertices.size(); ++j)
            {
                Vertex& vertex = blockVertices[addedFaceIndex.first + j];
                vertex.TexCoords.x += (texture_x * 0.0625f);
                vertex.TexCoords.y += (texture_z * 0.0625f);
            }
        }
        return std::pair(blockVertices, blockIndices);
    }
}


//#include "BlockMeshGenerator.h"
/*
std::vector<Vertex> BlockMeshGenerator::cubeVertices;
std::vector<unsigned int> BlockMeshGenerator::cubeIndices;
bool BlockMeshGenerator::isInitialized = false;*/


/*

    0   1
    3   2


    надо так стремиться каждую грань везд
    порядок     // 0 3 1   1 3 2
*/
/*
void Initialize()
{
    // 0 3 1   1 3 2
    cubeVertices =
    {
        // Back face (z = -0.5f) 
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}},        // TR
        {{-0.5f,  0.5f, -0.5f}, {0.0625f, 0.0f}},     // TL
        {{-0.5f, -0.5f, -0.5f}, {0.0625f, 0.0625f}},  // BL
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0625f}},     // BR

        // Front face (z = 0.5f) 
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},        // TL
        {{ 0.5f,  0.5f,  0.5f}, {0.0625f, 0.0f}},     // TR
        {{ 0.5f, -0.5f,  0.5f}, {0.0625f, 0.0625f}},  // BR
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0625f}},     // BL

        // Top face (y = 0.5f)
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}},        // BL (far left)
        {{ 0.5f,  0.5f, -0.5f}, {0.0625f, 0.0f}},     // BR (far right)
        {{ 0.5f,  0.5f,  0.5f}, {0.0625f, 0.0625f}},  // TR (near right)
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0625f}},     // TL (near left)

        // Bottom face (y = -0.5f)
        {{-0.5f,  -0.5f,  0.5f}, {0.0f, 0.0f}},       // TL
        {{ 0.5f,  -0.5f,  0.5f}, {0.0625f, 0.0f}},    // TR
        {{ 0.5f,  -0.5f, -0.5f}, {0.0625f, 0.0625f}}, // BR
        {{-0.5f,  -0.5f, -0.5f}, {0.0f, 0.0625f}},    // BL

        // Left face (x = -0.5f)
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}},        // TL
        {{-0.5f,  0.5f,  0.5f}, {0.0625f, 0.0f}},     // TR
        {{-0.5f, -0.5f,  0.5f}, {0.0625f, 0.0625f}},  // BR
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0625f}},     // BL

        // Right face (x = 0.5f)
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},        // TL
        {{ 0.5f,  0.5f, -0.5f}, {0.0625f, 0.0f}},     // TR
        {{ 0.5f, -0.5f, -0.5f}, {0.0625f, 0.0625f}},  // BR
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0625f}},     // BL

    };
}
*/
