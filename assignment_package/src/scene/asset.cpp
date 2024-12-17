#include "asset.h"
#include "terrain.h"
#include "chunk.h"


void setBlockSafe(const Terrain &t, int x, int y, int z, BlockType b) {
    if(t.hasChunkAt(x, z)) {
        const uPtr<Chunk> &c = t.getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setLocalBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                           static_cast<unsigned int>(y),
                           static_cast<unsigned int>(z - chunkOrigin.y),
                           b);
    }
}

void generateDefaultTree(const Terrain &terrain, int x, int y, int z) {
    // Generate trunk
    for (int i = y; i < y + 4; ++i) {
        setBlockSafe(terrain, x, i, z, WOOD); // Use helper function to safely place blocks
    }
    // Generate leaves
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dz = -2; dz <= 2; ++dz) {
            int distance = abs(dx) + abs(dz);
            if (distance <= 2) {
                setBlockSafe(terrain, x + dx, y + 3, z + dz, LEAF);
            }
            if (distance <= 1) {
                setBlockSafe(terrain, x + dx, y + 4, z + dz, LEAF);
            }
        }
    }
    setBlockSafe(terrain, x, y + 5, z, LEAF);

}

void generateDefaultTree2(const Terrain &terrain, int x, int y, int z)  {
    // Generate trunk
    for (int i = y; i < y + 6; ++i) {
        setBlockSafe(terrain, x, i, z, WOOD);
    }
    // Generate leaves
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            int distance = abs(dx) + abs(dz);
            if (distance <= 2) {
                setBlockSafe(terrain, x + dx, y + 4, z + dz, LEAF);
            }
            if (distance <= 1) {
                setBlockSafe(terrain, x + dx, y + 5, z + dz, LEAF);
            }
        }
    }
    setBlockSafe(terrain, x, y + 6, z, LEAF);


}


void generateDefaultSnowTree(const Terrain &terrain, int x, int y, int z) {
    //Generate trunk
    for (int i = y; i < y + 4; ++i) {
        setBlockSafe(terrain, x, i, z, WOOD);
    }
    // Generate leaves
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dz = -2; dz <= 2; ++dz) {
            int distance = abs(dx) + abs(dz);
            if (distance <= 2) {
                setBlockSafe(terrain, x + dx, y + 3, z + dz, SNOW_LEAF);
            }
            if (distance <= 1) {
                setBlockSafe(terrain, x + dx, y + 4, z + dz, SNOW_LEAF);
            }
        }
    }
    setBlockSafe(terrain, x, y + 5, z, SNOW_LEAF);

}


void generateDefaultSnowTree2(const Terrain &terrain, int x, int y, int z)  {
    // Generate trunk
    for (int i = y; i < y + 8; ++i) {
        setBlockSafe(terrain, x, i, z, WOOD);
    }

    int baseLayer = 3;
    for (int layer = 0; layer < 15; ++layer) { 
        int leafRadius = 3 - layer;
        if (layer % 2 == 0) {
            for (int dx = -leafRadius; dx <= leafRadius; ++dx) {
                for (int dz = -leafRadius; dz <= leafRadius; ++dz) {
                    if (abs(dx) + abs(dz) <= leafRadius) {
                        setBlockSafe(terrain, x + dx, y + baseLayer + layer + 1, z + dz, SNOW_LEAF);
                    }
                }
            }
        }
    }

    setBlockSafe(terrain, x, y + baseLayer + 5, z, SNOW_LEAF);  // Single block at the peak

}


void generateDeadSnowTree(const Terrain &terrain, int x, int y, int z)  {
    // Generate trunk
    for (int i = y; i < y + 8; ++i) {
        int random = rand() % 100;
        setBlockSafe(terrain, x, i, z, WOOD);
        if (i > y + 3) {
            if (random < 25) {
              setBlockSafe(terrain, x + 1, i, z, SNOW_LEAF);
            } else if (random < 50) {
                 setBlockSafe(terrain, x, i, z + 1, SNOW_LEAF);
            } else if (random < 75) {
                setBlockSafe(terrain, x, i, z - 1, SNOW_LEAF);
            } else {
                setBlockSafe(terrain, x - 1, i, z, SNOW_LEAF);
            }
        }
    }

    setBlockSafe(terrain, x, y + 8, z, WOOD);
    setBlockSafe(terrain, x, y + 9, z, WOOD);

}


void generateFallenTree(const Terrain &terrain, int x, int y, int z)  {
    // Generate trunk
    for (int i = z; i < z + 3; ++i) {
        setBlockSafe(terrain, x, y + 1, i, SIDE_WOOD);
    }


}


void generateCactus(const Terrain &terrain, int x, int y, int z) {
    int random = rand() % 100;

    if (random < 30) {
        setBlockSafe(terrain, x, y + 1, z, CACTUS);
    } else if (random < 70) {
        setBlockSafe(terrain, x, y + 1, z, CACTUS);
        setBlockSafe(terrain, x, y + 2, z, CACTUS);
    } else {
        setBlockSafe(terrain, x, y + 1, z, CACTUS);
        setBlockSafe(terrain, x, y + 2, z, CACTUS);
        setBlockSafe(terrain, x, y + 3, z, CACTUS);
    }
}

void generateDefaultSnowTree3(const Terrain &terrain, int x, int y, int z) {
    // Trunk height and base width
    int trunkHeight = 6;
    int trunkWidth = 1;
    int leafStartHeight = 4;

    // Generate the trunk
    for (int i = 0; i < trunkHeight; i++) {
        setBlockSafe(terrain, x, y + i, z, WOOD);  // WOOD is used for the trunk
    }

    // Generate the leaves (branches) and snow cover
    for (int height = leafStartHeight; height < trunkHeight; height++) {
        int leafRadius = trunkWidth + (trunkHeight - height);
        for (int angle = 0; angle < 360; angle += 20) {
            int dx = leafRadius * cos(angle * M_PI / 180);
            int dz = leafRadius * sin(angle * M_PI / 180);

            // Set blocks for leaves
            setBlockSafe(terrain, x + dx, y + height, z + dz, SNOW_LEAF);

            if (rand() % 2 == 0) {  // Random chance to place snow on leaves
                setBlockSafe(terrain, x + dx, y + height + 1, z + dz, SNOW); // Snow on top of leaves
            }
        }
    }
}


void generateSnowGrass(const Terrain &terrain, int x, int y, int z) {

    setBlockSafe(terrain, x, y, z, SNOW_GRASS_PATCH);

}

void generateDirtGrass(const Terrain &terrain, int x, int y, int z) {

    setBlockSafe(terrain, x, y, z, DIRT_GRASS_PATCH);

}


void generateSandCrack(const Terrain &terrain, int x, int y, int z) {

    setBlockSafe(terrain, x, y, z, SAND_CRACK);

}





