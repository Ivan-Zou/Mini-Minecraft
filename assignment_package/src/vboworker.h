#pragma once
#include "scene/chunk.h"
#include <QRunnable>
#include <QMutex>

class VBOWorker : public QRunnable {
private:
    Chunk *chunk;
    QMutex *vboMutex;
    std::vector<Chunk*> *bindToGPU;
public:
    VBOWorker(Chunk *chunk, QMutex *mutex, std::vector<Chunk*> *bindToGPU);
    void run() override;
};

