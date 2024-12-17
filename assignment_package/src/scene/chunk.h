#pragma once
#include <QMutex>
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include "drawable.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <unordered_set>


//using namespace std;

// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{

    EMPTY, GRASS, DIRT, STONE, WATER, SNOW, SAND, LAVA, BEDROCK, ICE, SNOW_DIRT, LEAF, WOOD,
    SNOW_LEAF, SAP, CACTUS, SIDE_WOOD, GRAVEL, SNOW_GRASS_PATCH, DIRT_GRASS_PATCH, COAL, LAPIS,
    COPPER, GOLD, SAND_CRACK

};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG, DIAG1, DIAG2
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};


// Store the VBO data of a chunk in an interleaved fashion
struct VBOdata {
    std::vector<glm::vec4> solidData, transData;
    std::vector<GLuint> solidIdx, transIdx;
};

struct Vertex {
    glm::vec4 pos, uv;

    Vertex(glm::vec4 pos, glm::vec4 uv)
        : pos(pos), uv(uv)
    {}
};

struct BlockFace {
    Direction dir;
    glm::ivec3 dirVector;
    std::array<Vertex, 4> vertices;

    BlockFace(Direction dir, glm::ivec3 dirVector, Vertex a, Vertex b, Vertex c, Vertex d)
        : dir(dir), dirVector(dirVector), vertices{a, b, c, d}
    {}
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    // The coordinates of the chunk's lower-left corner in world space
    int minX, minZ;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    VBOdata vboData;

public:
    Chunk(int x, int z, OpenGLContext* context);
    BlockType getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getLocalBlockAt(int x, int y, int z) const;
    void setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);
    glm::ivec2 getMin() const;
    std::vector<Chunk*> getNeighbors() const;

    // Buffers the VBO data in an interleaved fashion
    void createVBOdata() override;
    // Populates the vboData member of a chunk with the VBO data
    void generateVBOdata();
    // Updates the VBO with data of a face of a block
    void updateVBOdata(std::vector<glm::vec4>& vboData, std::vector<GLuint>& idx, int& vertCount, glm::vec4 blockPos, Direction dir, BlockType bType);
    // Checks if the block is a transparent block (ie. water)
    bool isTransparent(BlockType bType);

    //0 for grass, 1 for desert, 2 mountain
    int currBiome = -1;
    int prevBiome = -1;
};


const static std::array<BlockFace, 6> faces = {
    BlockFace(XPOS, glm::ivec3(1, 0, 0),
              Vertex(glm::vec4(1, 0, 1, 1), glm::vec4(0, 0, 0, 0)),
              Vertex(glm::vec4(1, 0, 0, 1), glm::vec4(0.0625, 0, 0, 0)),
              Vertex(glm::vec4(1, 1, 0, 1), glm::vec4(0.0625, 0.0625, 0, 0)),
              Vertex(glm::vec4(1, 1, 1, 1), glm::vec4(0, 0.0625, 0, 0))),
    BlockFace(XNEG, glm::ivec3(-1, 0, 0),
              Vertex(glm::vec4(0, 0, 0, 1), glm::vec4(0, 0, 0, 0)),
              Vertex(glm::vec4(0, 0, 1, 1), glm::vec4(0.0625, 0, 0, 0)),
              Vertex(glm::vec4(0, 1, 1, 1), glm::vec4(0.0625, 0.0625, 0, 0)),
              Vertex(glm::vec4(0, 1, 0, 1), glm::vec4(0, 0.0625, 0, 0))),
    BlockFace(YPOS, glm::ivec3(0, 1, 0),
              Vertex(glm::vec4(0, 1, 1, 1), glm::vec4(0, 0, 0, 0)),
              Vertex(glm::vec4(1, 1, 1, 1), glm::vec4(0.0625, 0, 0, 0)),
              Vertex(glm::vec4(1, 1, 0, 1), glm::vec4(0.0625, 0.0625, 0, 0)),
              Vertex(glm::vec4(0, 1, 0, 1), glm::vec4(0, 0.0625, 0, 0))),
    BlockFace(YNEG, glm::ivec3(0, -1, 0),
              Vertex(glm::vec4(0, 0, 0, 1), glm::vec4(0, 0, 0, 0)),
              Vertex(glm::vec4(1, 0, 0, 1), glm::vec4(0.0625, 0, 0, 0)),
              Vertex(glm::vec4(1, 0, 1, 1), glm::vec4(0.0625, 0.0625, 0, 0)),
              Vertex(glm::vec4(0, 0, 1, 1), glm::vec4(0, 0.0625, 0, 0))),
    BlockFace(ZPOS, glm::ivec3(0, 0, 1),
              Vertex(glm::vec4(0, 0, 1, 1), glm::vec4(0, 0, 0, 0)),
              Vertex(glm::vec4(1, 0, 1, 1), glm::vec4(0.0625, 0, 0, 0)),
              Vertex(glm::vec4(1, 1, 1, 1), glm::vec4(0.0625, 0.0625, 0, 0)),
              Vertex(glm::vec4(0, 1, 1, 1), glm::vec4(0, 0.0625, 0, 0))),
    BlockFace(ZNEG, glm::ivec3(0, 0, -1),
              Vertex(glm::vec4(1, 0, 0, 1), glm::vec4(0, 0, 0, 0)),
              Vertex(glm::vec4(0, 0, 0, 1), glm::vec4(0.0625, 0, 0, 0)),
              Vertex(glm::vec4(0, 1, 0, 1), glm::vec4(0.0625, 0.0625, 0, 0)),
              Vertex(glm::vec4(1, 1, 0, 1), glm::vec4(0, 0.0625, 0, 0)))
};

const static std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec4, EnumHash>, EnumHash> blockUVs {
    {GRASS, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                               {XNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(8.f/16.f, 13.f/16.f, 0, 0)},
                                                               {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)}}},

    {DIRT, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)}}},

    {STONE, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                               {XNEG, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                               {YNEG, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZPOS, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(1.f/16.f, 15.f/16.f, 0, 0)}}},

    {WATER, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                               {XNEG, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(13.f/16.f, 3.f/16.f, 0, 1)},
                                                               {YNEG, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                               {ZPOS, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(13.f/16.f, 3.f/16.f, 0, 0)}}},

    {SNOW, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)}}},

    {SAND, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)}}},

    {LAVA, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(13.f/16.f, 1.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(13.f/16.f, 1.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(13.f/16.f, 1.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(13.f/16.f, 1.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(13.f/16.f, 1.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(13.f/16.f, 1.f/16.f, 0, 0)}}},

    {BEDROCK, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(1.f/16.f, 14.f/16.f, 0, 0)},
                                                                 {XNEG, glm::vec4(1.f/16.f, 14.f/16.f, 0, 0)},
                                                                 {YPOS, glm::vec4(1.f/16.f, 14.f/16.f, 0, 0)},
                                                                 {YNEG, glm::vec4(1.f/16.f, 14.f/16.f, 0, 0)},
                                                                 {ZPOS, glm::vec4(1.f/16.f, 14.f/16.f, 0, 0)},
                                                                 {ZNEG, glm::vec4(1.f/16.f, 14.f/16.f, 0, 0)}}},
    {ICE, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(3.f/16.f, 11.f/16.f, 0, 0)},
                                                             {XNEG, glm::vec4(3.f/16.f, 11.f/16.f, 0, 0)},
                                                             {YPOS, glm::vec4(3.f/16.f, 11.f/16.f, 0, 0)},
                                                             {YNEG, glm::vec4(3.f/16.f, 11.f/16.f, 0, 0)},
                                                             {ZPOS, glm::vec4(3.f/16.f, 11.f/16.f, 0, 0)},
                                                             {ZNEG, glm::vec4(3.f/16.f, 11.f/16.f, 0, 0)}}},
    {SNOW_DIRT, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(4.f/16.f, 11.f/16.f, 0, 0)},
                                                                   {XNEG, glm::vec4(4.f/16.f, 11.f/16.f, 0, 0)},
                                                                   {YPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                                   {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                   {ZPOS, glm::vec4(4.f/16.f, 11.f/16.f, 0, 0)},
                                                                   {ZNEG, glm::vec4(4.f/16.f, 11.f/16.f, 0, 0)}}},
    {LEAF, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)}}},

    {WOOD, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(5.f/16.f, 14.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(5.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)}}},
    {SIDE_WOOD, std::unordered_map<Direction, glm::vec4, EnumHash>{  {XPOS, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(4.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(5.f/16.f, 14.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(5.f/16.f, 14.f/16.f, 0, 0)}}},
    {SNOW_LEAF, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(2.f/16.f, 11.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(5.f/16.f, 12.f/16.f, 0, 0)}}},
    {CACTUS, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(6.f/16.f, 11.f/16.f, 0, 3)},
                                                              {XNEG, glm::vec4(6.f/16.f, 11.f/16.f, 0, -3)},
                                                              {YPOS, glm::vec4(5.f/16.f, 11.f/16.f, 0, 2)},
                                                              {YNEG, glm::vec4(5.f/16.f, 11.f/16.f, 0, -2)},
                                                              {ZPOS, glm::vec4(6.f/16.f, 11.f/16.f, 0, 4)},
                                                              {ZNEG, glm::vec4(6.f/16.f, 11.f/16.f, 0, -4)}}},
    {GRAVEL, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(0.f/16.f, 15.f/16.f, 0, 0)},
                                                               {XNEG, glm::vec4(0.f/16.f, 15.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(0.f/16.f, 15.f/16.f, 0, 0)},
                                                               {YNEG, glm::vec4(0.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZPOS, glm::vec4(0.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(0.f/16.f, 15.f/16.f, 0, 0)}}},
    {SNOW_GRASS_PATCH, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                               {XNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(1.f/16.f, 6.f/16.f, 0, -7)},
                                                               {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)}}},
    {DIRT_GRASS_PATCH, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                                          {XNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                                          {YPOS, glm::vec4(2.f/16.f, 5.f/16.f, 0, -8)},
                                                                          {YNEG, glm::vec4(2.f/16.f, 15.f/16.f, 0, 0)},
                                                                          {ZPOS, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)},
                                                                          {ZNEG, glm::vec4(3.f/16.f, 15.f/16.f, 0, 0)}}},
    {COAL, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f/16.f, 13.f/16.f, 0, 0)},
                                                                {XNEG, glm::vec4(2.f/16.f, 13.f/16.f, 0, 0)},
                                                                {YPOS, glm::vec4(2.f/16.f, 13.f/16.f, 0, 0)},
                                                                {YNEG, glm::vec4(2.f/16.f, 13.f/16.f, 0, 0)},
                                                                {ZPOS, glm::vec4(2.f/16.f, 13.f/16.f, 0, 0)},
                                                                {ZNEG, glm::vec4(2.f/16.f, 13.f/16.f, 0, 0)}}},
    {LAPIS, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(0.f/16.f, 5.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(0.f/16.f, 5.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(0.f/16.f, 5.f/16.f, 0, 0)},
                                                              {YNEG, glm::vec4(0.f/16.f, 5.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(0.f/16.f, 5.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(0.f/16.f, 5.f/16.f, 0, 0)}}},
    {COPPER, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(1.f/16.f, 13.f/16.f, 0, 0)},
                                                               {XNEG, glm::vec4(1.f/16.f, 13.f/16.f, 0, 0)},
                                                               {YPOS, glm::vec4(1.f/16.f, 13.f/16.f, 0, 0)},
                                                               {YNEG, glm::vec4(1.f/16.f, 13.f/16.f, 0, 0)},
                                                               {ZPOS, glm::vec4(1.f/16.f, 13.f/16.f, 0, 0)},
                                                               {ZNEG, glm::vec4(1.f/16.f, 13.f/16.f, 0, 0)}}},
    {GOLD, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(0.f/16.f, 13.f/16.f, 0, 0)},
                                                                {XNEG, glm::vec4(0.f/16.f, 13.f/16.f, 0, 0)},
                                                                {YPOS, glm::vec4(0.f/16.f, 13.f/16.f, 0, 0)},
                                                                {YNEG, glm::vec4(0.f/16.f, 13.f/16.f, 0, 0)},
                                                                {ZPOS, glm::vec4(0.f/16.f, 13.f/16.f, 0, 0)},
                                                                {ZNEG, glm::vec4(0.f/16.f, 13.f/16.f, 0, 0)}}},
    {SAND_CRACK, std::unordered_map<Direction, glm::vec4, EnumHash>{{XPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {XNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {YPOS, glm::vec4(0.f/16.f, 2.f/16.f, 0, -9)},
                                                              {YNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZPOS, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)},
                                                              {ZNEG, glm::vec4(2.f/16.f, 14.f/16.f, 0, 0)}}},

    };

