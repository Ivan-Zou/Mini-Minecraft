#pragma once
#include "scene/terrain.h"
#include <QRunnable>
#include <QMutex>

class BlockWorker : public QRunnable {
private:
    Terrain *terrain;
    QMutex *blockMutex;
    std::unordered_set<Chunk*> *needVBO;
    int x;
    int z;
public:
    BlockWorker(Terrain* t, QMutex *mutex, int x, int z, std::unordered_set<Chunk*> *needVBO);
    void run() override;
};
