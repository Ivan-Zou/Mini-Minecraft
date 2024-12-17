#include "vboworker.h"

VBOWorker::VBOWorker(Chunk* chunk, QMutex * mutex, std::vector<Chunk*> *bindToGPU)
    : chunk(chunk), vboMutex(mutex), bindToGPU(bindToGPU)
{}

void VBOWorker::run() {
    try {
        chunk->generateVBOdata();
        vboMutex->lock();
        bindToGPU->push_back(chunk);
        vboMutex->unlock();
    } catch (...) {
        return;
    }
}
