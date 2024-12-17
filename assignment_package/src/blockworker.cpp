#include "blockworker.h"
#include <iostream>

BlockWorker::BlockWorker(Terrain* t, QMutex *mutex, int x, int z, std::unordered_set<Chunk*> *needVBO)
    : terrain(t), blockMutex(mutex), needVBO(needVBO), x(x), z(z)
{}

void BlockWorker::run() {
    try {
        if (terrain->hasChunkAt(x, z)) {
            Chunk* cPtr = terrain->getChunkAt(x, z).get();
            terrain->fillChunk(cPtr, x, z);
            blockMutex->lock();
            needVBO->insert(cPtr);
            blockMutex->unlock();
        }
    } catch (...) {
        return;
    }
}
