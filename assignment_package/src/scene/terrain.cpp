#include "terrain.h"
#include "cube.h"
#include <iostream>
#include "noise.h"
#include <stdexcept>
#include "chunk.h"
#include "asset.h"

#include <QMediaPlayer>
#include <QAudioOutput>


Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), m_geomCube(context),
    mp_context(context)
{}

Terrain::~Terrain() {
    m_geomCube.destroyVBOdata();
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getGlobalBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                                  static_cast<unsigned int>(y),
                                  static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

BlockType Terrain::getGlobalBlockAt(glm::vec3 p) const {
    return getGlobalBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    auto result =  m_chunks.find(toKey(16 * xFloor, 16 * zFloor));// != m_chunks.end();
    if(result == m_chunks.end()) {
        return false;
    }
    else {
        return m_chunks.at(toKey(16 * xFloor, 16 * zFloor)) != nullptr;
    }
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setGlobalBlockAt(int x, int y, int z, BlockType t)
{
    if(hasChunkAt(x, z)) {
        uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                           static_cast<unsigned int>(y),
                           static_cast<unsigned int>(z - chunkOrigin.y),
                           t);
        // Reset VBO data
        c->destroyVBOdata();
        c->generateVBOdata();
        c->createVBOdata();

    } else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(x, z, mp_context);
    Chunk *cPtr = chunk.get();
    int64_t key = toKey(x, z);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }
    // Add new chunk to shared data structures
    m_chunks[toKey(x, z)] = std::move(chunk);
    m_generatedTerrain.insert(key);
    return cPtr;
}


void Terrain::draw(int minX, int maxX, int minZ, int maxZ, ShaderProgram *shaderProgram) {
    // Draw Solid Blocks First
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                if (chunk->elemCount(INDEX) > 0) {
                    shaderProgram->drawInterleaved(*chunk, false);
                }
            }
        }
    }
    // Draw Transparent Blocks After
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16) {
            if (hasChunkAt(x, z)) {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                if (chunk->elemCount(TRANSPARENT_INDEX) > 0) {
                    shaderProgram->drawInterleaved(*chunk, true);
                }
            }
        }
    }
}

float Terrain::mapToUnitInterval(float x, float min, float max) const {
    return (x - min) / (max - min);
}

float Terrain::getTemperature(float x, float z) const {
    return voronoi(x, z, 750);
}


float Terrain::getMoisture(float x, float z) const {
    float noise = mapToUnitInterval(perlin(x, z, 750), -1, 1);
    return glm::smoothstep(0.2f, 0.8f, noise);
}


float Terrain::getMountainHeight(float x, float z) const {
    float v = 1 - abs(fbm(x, z, 200.f, 4, voronoi));
    float base = perlin(x, z);
    v = glm::clamp(pow(v * 1.5, 5.f), 0.0, 1.0);
    return 135.f + (base * 20.f) + (v * 100.f);
}

float Terrain::getFlatHeight(float x, float z) const {
    float v = fbm(x, z, 80, 3, voronoi);
    int steps = 50;
    float additionalHeight = (round(v * steps) / steps) * 30.f;
    return 128.f + additionalHeight;
}

float Terrain::getSnowyPlainsHeight(float x, float z) const {
    float p = fbm(x, z, 120.f, 3, voronoi);
    int steps = 100;
    p = glm::smoothstep(0.1, 0.9, round(p * steps) / steps);
    return 135.f + (p * 35.f);
}

float Terrain::getTerrainHeight(float x, float z, float moisture, float temperature) const {
    // Get height of all biomes at current position
    float mountainHeight = getMountainHeight(x, z);
    float flatHeight = getFlatHeight(x, z);
    float snowyPlainsHeight = getSnowyPlainsHeight(x, z);
    moisture = glm::smoothstep(0.4f, 0.6f, moisture);
    temperature = glm::smoothstep(0.4f, 0.6f, temperature);
    float h1 = glm::mix(mountainHeight, flatHeight, moisture);
    float h2 = glm::mix(flatHeight + 10.f, snowyPlainsHeight, moisture);
    return glm::mix(h1, h2, temperature);

}

float Terrain::getCaveHeight(float x, float y, float z) const {

    float caveFrequencyX = 102.0f;
    float caveFrequencyY = 108.0f;
    float caveFrequencyZ = 102.0f;

    // Generate Perlin noise with frequencies
    float noise1 = perlin3D(x / caveFrequencyX, y / caveFrequencyY, z / caveFrequencyZ);
    float noise2 = perlin3D((x) / 34.0f, (y) / 33.0f, (z) / 34.0f);

    return (0.10*noise1 + 0.90*noise2);
}


float Terrain::getGrassHeight(float x, float y, float z) const {

    float grassFrequencyX = 102.0f;
    float grassFrequencyY = 108.0f;
    float grassFrequencyZ = 102.0f;

    // Generate Perlin noise with frequencies
    float noise1 = perlin3D(x / grassFrequencyX, y / grassFrequencyY, z / grassFrequencyZ);
    float noise2 = perlin3D((x) / 34.0f, (y) / 33.0f, (z) / 34.0f);

    return (0.05*noise1 + 0.95*noise2);
}


float Terrain::getTreeHeight(float x, float z) const {
    float noise = simpleFBM(x, z);
    return noise;
 }


BlockType Terrain::getMountainBlock(int y, int top) const {
    return (y == top && top >= 200) ? SNOW : STONE;
}

BlockType Terrain::getBlockType(BiomeType b, int y, int top) const {
    if (y == 0) {
        return BEDROCK;
    } else if (y <= 25) {
        return LAVA;
    } else if (y <= 135){
        return STONE;
    } else if (b == SNOWY_PLAINS) {
        return (y == top) ? SNOW_DIRT : DIRT;
    } else if (b == MOUNTAIN) {
        return getMountainBlock(y, top);
    } else if (b == DESERT) {
        return SAND;
    } else {
        return (y == top) ? GRASS : DIRT;
    }
}

BiomeType Terrain::getBiomeType(float moisture, float temp) const {
    if (moisture < 0.5) {
        return (temp < 0.5) ? MOUNTAIN : DESERT;
    } else {
        return (temp < 0.5) ? SNOWY_PLAINS : GRASSLAND;
    }
}



void Terrain::fillChunk(Chunk* c, int x, int z) const {
    for (int dx = 0; dx < 16; ++dx) {
        for (int dz = 0; dz < 16; ++dz) {
            int currX = x + dx, currZ = z + dz;

            float moisture = getMoisture(currX, currZ);
            float temperature = getTemperature(currX, currZ);
            float h = getTerrainHeight(currX, currZ, moisture, temperature);
            BiomeType b = getBiomeType(moisture, temperature);


            for (int y = 0; y <= 25; ++y) {
              //  setBlockSafe(dx, y, dz, LAVA);
                c->setLocalBlockAt(dx, y, dz, LAVA);
            }
            int top = floor(h);

            for (int y = 0; y <= top; ++y) {
                bool isEmpty = makeCaves(currX, y, currZ) && b == MOUNTAIN;
                if (isEmpty) {
                    // If it's a cave, but the block would be LAVA, keep it as LAVA
                    if (getBlockType(b, y, top) == LAVA) {
                        c->setLocalBlockAt(dx, y, dz, LAVA);
                    } else {
                        c->setLocalBlockAt(dx, y, dz, EMPTY); // Otherwise, mark it as empty
                    }
                } else {

                    int random = rand() % 100;
                    if (random < 80) {
                        c->setLocalBlockAt(dx, y, dz, getBlockType(b, y, top));
                    } else if (b == MOUNTAIN && random < 98) {
                        c->setLocalBlockAt(dx, y, dz, GRAVEL);
                    }else {
                        random = rand() % 100;
                        if (b == MOUNTAIN && (y < 140 && y > 27)) {
                            if (random < 85) {
                                c->setLocalBlockAt(dx, y, dz, COAL);
                            } else if (random < 95) {
                                c->setLocalBlockAt(dx, y, dz, COPPER);
                            } else if (random < 98) {
                                c->setLocalBlockAt(dx, y, dz, GOLD);
                            } else {
                                c->setLocalBlockAt(dx, y, dz, LAPIS);
                            }
                        } else {
                             c->setLocalBlockAt(dx, y, dz, getBlockType(b, y, top));
                        }




                    }
                }
            }
            // Set EMPTY blocks to water/ice
            if (top < 138) {
                for (int y = top; y <= 138; ++y) {
                    c->setLocalBlockAt(dx, y, dz, b == SNOWY_PLAINS ? ICE : WATER);
                }
            }


            //Asset generation
            float genTree = makeTrees(currX, top, currZ);

            //Create Grass Patches
            if ((b == SNOWY_PLAINS || b == MOUNTAIN) && makeGrass(currX, top, currZ) && top >= 138) {
                generateSnowGrass(*this, currX, top, currZ);
            } else if (b == GRASSLAND && makeGrass(currX, top, currZ) && top >= 138) {
                generateDirtGrass(*this, currX, top, currZ);
            } else if (b == DESERT &&  makeGrass(currX, top, currZ) && top >= 138) {
                generateSandCrack(*this, currX, top, currZ);
            }


            if (genTree > 0.85 && b == GRASSLAND && top >= 138 ) {

                int random = rand() % 100;
                if (random < 50) {
                    generateDefaultTree(*this, currX, top, currZ);
                  //  generateFallenTree(*this, currX, top, currZ);
                } else if (random < 90) {
                    generateDefaultTree2(*this, currX, top, currZ);
                } else {
                    generateFallenTree(*this, currX, top, currZ);
                }
            } else if (genTree > 0.87 && b == SNOWY_PLAINS && top >= 138) {
                int random = rand() % 100;
                if (random < 30) {
                    generateDefaultSnowTree(*this, currX, top, currZ);
                } else if (random < 60){
                    generateDefaultSnowTree2(*this, currX, top, currZ);
                } else if (random < 90){
                    generateDeadSnowTree(*this, currX, top, currZ);
                } else {
                    generateDefaultSnowTree3(*this, currX, top, currZ);
                }
            } else if (genTree > 0.89 && b == DESERT && top >= 138) {
                    generateCactus(*this, currX, top, currZ);
            }
        }
    }
}

bool Terrain::makeGrass(int x, int y, int z) const {
    float perlinNoise = getGrassHeight(x, y, z);
    return perlinNoise >= 25.f;
}
float Terrain::makeTrees(int x, int top, int z) const{
    float t = getTreeHeight(x, z);
    return t;
}


bool Terrain::makeCaves(int x, int y, int z) const {
    if ((y >= 130) && (y < 150)) {
        float perlinNoise = getCaveHeight(x, y, z);
        return perlinNoise >= 25.f;
    } else {
        return false;
    }
}



