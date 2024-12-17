#pragma once
#include <QMutex>
#include "smartpointerhelp.h"
#include "chunk.h"
#include <unordered_map>
#include <unordered_set>
#include "shaderprogram.h"
#include "cube.h"
#include <unordered_set>


enum BiomeType : unsigned char
{
    MOUNTAIN, GRASSLAND, DESERT, SNOWY_PLAINS
};

// Helper functions to convert (x, z) to and from hash map key
int64_t toKey(int x, int z);
glm::ivec2 toCoords(int64_t k);

// The container class for all of the Chunks in the game.
// Ultimately, while Terrain will always store all Chunks,
// not all Chunks will be drawn at any given time as the world
// expands.
class Terrain {
private:
    // Stores every Chunk according to the location of its lower-left corner
    // in world space.
    // We combine the X and Z coordinates of the Chunk's corner into one 64-bit int
    // so that we can use them as a key for the map, as objects like std::pairs or
    // glm::ivec2s are not hashable by default, so they cannot be used as keys.
    std::unordered_map<int64_t, uPtr<Chunk>> m_chunks;

    // We will designate every 64 x 64 area of the world's x-z plane
    // as one "terrain generation zone". Every time the player moves
    // near a portion of the world that has not yet been generated
    // (i.e. its lower-left coordinates are not in this set), a new
    // 4 x 4 collection of Chunks is created to represent that area
    // of the world.
    // The world that exists when the base code is run consists of exactly
    // one 64 x 64 area with its lower-left corner at (0, 0).
    // When milestone 1 has been implemented, the Player can move around the
    // world to add more "terrain generation zone" IDs to this set.
    // While only the 3 x 3 collection of terrain generation zones
    // surrounding the Player should be rendered, the Chunks
    // in the Terrain will never be deleted until the program is terminated.
    std::unordered_set<int64_t> m_generatedTerrain;

    Cube m_geomCube;


    OpenGLContext* mp_context;


    // Helper Methods
    float mapToUnitInterval(float x, float min, float max) const;

    // Biome Generation
    float getTemperature(float x, float z) const;
    float getMoisture(float x, float z) const;

    // Terrain Generation
    float getMountainHeight(float x, float z) const;
    float getFlatHeight(float x, float z) const;
    float getSnowyPlainsHeight(float x, float z) const;
    float getTerrainHeight(float x, float z, float moisture, float temperature) const;

    // Fill Chunks
    BlockType getMountainBlock(int y, int top) const;
    BlockType getBlockType(BiomeType b, int y, int top) const;
    BiomeType getBiomeType(float moisture, float temp) const;

    //Cave generation
    bool makeCaves(int x, int y, int z) const;
    float getCaveHeight(float x, float y, float z) const;

    //Tree Generation
    void generateSampleTree(int x, int y, int z) const;
    float makeTrees(int x, int top, int z) const;
    float getTreeHeight(float x, float z) const;
    void setBlockSafe(int x, int y, int z, BlockType type) const;

    //Grass Patches Generation
    bool makeGrass(int x, int y, int z) const;
    float getGrassHeight(float x, float y, float z) const;


public:
    Terrain(OpenGLContext *context);
    ~Terrain();
    // Instantiates a new Chunk and stores it in
    // our chunk map at the given coordinates.
    Chunk* instantiateChunkAt(int x, int z);
    void fillChunk(Chunk* c, int x, int z) const;
    // Do these world-space coordinates lie within
    // a Chunk that exists?
    bool hasChunkAt(int x, int z) const;
    // Assuming a Chunk exists at these coords,
    // return a mutable reference to it
    uPtr<Chunk>& getChunkAt(int x, int z);
    // Assuming a Chunk exists at these coords,
    // return a const reference to it
    const uPtr<Chunk>& getChunkAt(int x, int z) const;
    // Given a world-space coordinate (which may have negative
    // values) return the block stored at that point in space.
    BlockType getGlobalBlockAt(int x, int y, int z) const;
    BlockType getGlobalBlockAt(glm::vec3 p) const;
    // Given a world-space coordinate (which may have negative
    // values) set the block at that point in space to the
    // given type.
    void setGlobalBlockAt(int x, int y, int z, BlockType t);

    // Draws every Chunk that falls within the bounding box
    // described by the min and max coords, using the provided
    // ShaderProgram
    void draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram);

    //0 for grass, 1 for desert, 2 mountain
    int currBiome = -1;
    int prevBiome = -1;

};
