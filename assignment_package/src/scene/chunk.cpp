#include "chunk.h"
#include <iostream>

Chunk::Chunk(int x, int z, OpenGLContext *context)
    : Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}},
    vboData()
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
}

// Does bounds checking with at()
BlockType Chunk::getLocalBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    if (x >= 16 || x < 0 || y >= 256 || y < 0 || z >= 16 || z < 0) return EMPTY;
    return m_blocks.at(x + 16 * y + 16 * 256 * z);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getLocalBlockAt(int x, int y, int z) const {
    return getLocalBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setLocalBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}


const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

std::vector<Chunk*> Chunk::getNeighbors() const {
    std::vector<Chunk*> neighbors;
    for (auto it = m_neighbors.begin(); it != m_neighbors.end(); ++it) {
        if (it->second != nullptr) {
            neighbors.push_back(it->second);
        }
    }
    return neighbors;
}

void Chunk::createVBOdata() {
    std::vector<glm::vec4> solidData = vboData.solidData;
    std::vector<GLuint> solidIdx = vboData.solidIdx;
    std::vector<glm::vec4> transData = vboData.transData;
    std::vector<GLuint> transIdx = vboData.transIdx;

    // Set Buffer index counts
    indexCounts[INDEX] = solidIdx.size();
    indexCounts[TRANSPARENT_INDEX] = transIdx.size();

    // Create and bind interleaved buffer
    generateBuffer(INTERLEAVED);
    bindBuffer(INTERLEAVED);
    mp_context->glBufferData(GL_ARRAY_BUFFER,
                             solidData.size() * sizeof(glm::vec4),
                             solidData.data(),
                             GL_STATIC_DRAW);
    generateBuffer(TRANSPARENT_INTERLEAVED);
    bindBuffer(TRANSPARENT_INTERLEAVED);
    mp_context->glBufferData(GL_ARRAY_BUFFER,
                             transData.size() * sizeof(glm::vec4),
                             transData.data(),
                             GL_STATIC_DRAW);

    // Create and bind index buffer
    generateBuffer(INDEX);
    bindBuffer(INDEX);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             solidIdx.size() * sizeof(GLuint),
                             solidIdx.data(),
                             GL_STATIC_DRAW);
    generateBuffer(TRANSPARENT_INDEX);
    bindBuffer(TRANSPARENT_INDEX);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             transIdx.size() * sizeof(GLuint),
                             transIdx.data(),
                             GL_STATIC_DRAW);

}

void Chunk::generateVBOdata() {
    // Create vectors to store the VBO data
    std::vector<glm::vec4> solidData, transData;
    std::vector<GLuint> solidIdx, transIdx;

    int solidVertCount = 0;
    int transVertCount = 0;

    // Iterate through each block in the chunk
    for (int z = 0; z < 16; ++z) {
        for (int y = 0; y < 256; ++y) {
            for (int x = 0; x < 16; ++x) {
                BlockType currBlock = getLocalBlockAt(x, y, z);
                // if the current block is nothing, go to the next iteration
                if (currBlock == EMPTY) continue;

                glm::vec4 blockPos = glm::vec4(x, y, z, 0.f) + glm::vec4(minX, 0.f, minZ, 0.f);
                // Get neighboring blocks
                BlockType xPos = getLocalBlockAt(x + 1, y, z);
                BlockType xNeg = getLocalBlockAt(x - 1, y, z);
                BlockType yPos = getLocalBlockAt(x, y + 1, z);
                BlockType yNeg = getLocalBlockAt(x, y - 1, z);
                BlockType zPos = getLocalBlockAt(x, y, z + 1);
                BlockType zNeg = getLocalBlockAt(x, y, z - 1);
                // Check for neighboring blocks over the edges of this chunk and set accordingly
                if (x == 0 && m_neighbors.find(XNEG) != m_neighbors.end() && m_neighbors[XNEG]) xNeg = m_neighbors[XNEG]->getLocalBlockAt(15, y, z);
                if (x == 15 && m_neighbors.find(XPOS) != m_neighbors.end() && m_neighbors[XPOS]) xPos = m_neighbors[XPOS]->getLocalBlockAt(0, y, z);
                if (y == 0) yNeg = EMPTY;
                if (y == 255) yPos = EMPTY;
                if (z == 0 && m_neighbors.find(ZNEG) != m_neighbors.end() && m_neighbors[ZNEG]) zNeg = m_neighbors[ZNEG]->getLocalBlockAt(x, y, 15);
                if (z == 15 && m_neighbors.find(ZPOS) != m_neighbors.end() && m_neighbors[ZPOS]) zPos = m_neighbors[ZPOS]->getLocalBlockAt(x, y, 0);

                // Create an array of the neighbors so we can loop over them
                std::array<std::pair<Direction, BlockType>, 6> neighbors = {
                    std::pair(XPOS, xPos), std::pair(XNEG, xNeg),
                    std::pair(YPOS, yPos), std::pair(YNEG, yNeg),
                    std::pair(ZPOS, zPos), std::pair(ZNEG, zNeg)
                };
                // Loop Over Neighbors
                for (auto neighbor : neighbors) {
                    BlockType neighborType = neighbor.second;
                    // If the neighbor is empty or (the neighbor is transparent and the current block isn't the
                    // same block as the neighbor), then add to VBO to be drawn
                    if (neighborType == EMPTY ||(isTransparent(neighborType) && neighborType != currBlock) ||
                        neighborType == CACTUS) {

                        if (isTransparent(currBlock)) {
                            updateVBOdata(transData, transIdx, transVertCount, blockPos, neighbor.first, currBlock);
                        } else {
                            // otherwise, add to the solid vectors
                            updateVBOdata(solidData, solidIdx, solidVertCount, blockPos, neighbor.first, currBlock);

                        }

                    }
                }
            }
        }
    }

    // Set the VBO data to the vboData member
    vboData.solidData = solidData;
    vboData.solidIdx = solidIdx;
    vboData.transData = transData;
    vboData.transIdx = transIdx;
}

void Chunk::updateVBOdata(std::vector<glm::vec4>& vboData, std::vector<GLuint>& idx, int& vertCount, glm::vec4 blockPos, Direction dir, BlockType bType) {
    BlockFace face = faces.at(dir);
    glm::vec4 uv = blockUVs.at(bType).at(dir);
    glm::vec4 nor = glm::vec4(face.dirVector, 1);

    // If the block type is water or lava, set the third value of the uv vec4
    // to 1 to represent that this block is animateable
    if (bType == WATER || bType == LAVA) {
        uv.z = 1;
    }



    for (int i = 0; i < 4; ++i) {
        // add position to VBO
        vboData.push_back(face.vertices.at(i).pos + blockPos);
        // add normal to VBO
        vboData.push_back(nor);
        // add uv to VBO
        vboData.push_back(uv + face.vertices.at(i).uv);
    }

    // add indices to idx vector
    idx.push_back(vertCount);
    idx.push_back(vertCount + 1);
    idx.push_back(vertCount + 2);
    idx.push_back(vertCount);
    idx.push_back(vertCount + 2);
    idx.push_back(vertCount + 3);
    vertCount += 4;
}

bool Chunk::isTransparent(BlockType bType) {
    return bType == WATER || bType == ICE;
}

glm::ivec2 Chunk::getMin() const {
    return glm::ivec2(minX, minZ);
}
