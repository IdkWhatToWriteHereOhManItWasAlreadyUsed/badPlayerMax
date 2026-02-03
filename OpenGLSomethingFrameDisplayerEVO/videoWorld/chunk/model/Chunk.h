#pragma once

#include <array>
#include <videoWorld/block/Block.h>

const int CHUNK_WIDTH = 16;
const int CHUNK_LENGTH = 16;
const int CHUNK_HEIGHT = 30;

template<typename T>
using ChunkModelArray = std::array<std::array<std::array<T, CHUNK_HEIGHT>, CHUNK_WIDTH>, CHUNK_LENGTH>;

class Chunk
{
	ChunkModelArray<Block> m_blocks;
public:
	Chunk();
	~Chunk();
	// координаты чанка в чанках (НЕ в блоках)
	int x = 0, z = 0;
	
	// другие параметры
	const ChunkModelArray<Block>& GetBlocks() const { return m_blocks;} 
	void SetBlock(int x, int z, int y, const Block block);
	Block* GetBlock(int x, int z, int y) const;  
};
