#pragma once
#include "entity.h"
#include "camera.h"
#include "terrain.h"
#include "QSoundEffect"
class Player : public Entity {
private:
    glm::vec3 m_velocity, m_acceleration;
    Camera m_camera;
    const Terrain &mcr_terrain;

    void processInputs(InputBundle &inputs);
    void computePhysics(float dT, const Terrain &terrain);
    float maxVel = 3;
    bool flightMode = true;
    bool prevCollisionZ = false;
    bool onGround = false;
    bool breakBlock = false;
    bool onLava = false;
    bool onWater = false;
    bool caminLava = false;
    bool caminWater = false;
    bool playerMove = false;

    //Private helper
    bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit);
    glm::vec3 normalizeVelocity();

public:
    // Readonly public reference to our camera
    // for easy access from MyGL
    const Camera& mcr_camera;

     bool isSoundLoaded = false;

    Player(glm::vec3 pos, const Terrain &terrain);
    virtual ~Player() override;

    void setCameraWidthHeight(unsigned int w, unsigned int h);

    void tick(float dT, InputBundle &input) override;

    // Player overrides all of Entity's movement
    // functions so that it transforms its camera
    // by the same amount as it transforms itself.
    void moveAlongVector(glm::vec3 dir) override;
    void moveForwardLocal(float amount) override;
    void moveRightLocal(float amount) override;
    void moveUpLocal(float amount) override;
    void moveForwardGlobal(float amount) override;
    void moveRightGlobal(float amount) override;
    void moveUpGlobal(float amount) override;
    void rotateOnForwardLocal(float degrees) override;
    void rotateOnRightLocal(float degrees) override;
    void rotateOnUpLocal(float degrees) override;
    void rotateOnForwardGlobal(float degrees) override;
    void rotateOnRightGlobal(float degrees) override;
    void rotateOnUpGlobal(float degrees) override;

    void toggleFlightMode() {
        this->flightMode = !this->flightMode;
    }

    glm::vec3 getBlock(const Terrain &terrain);
    glm::vec3 placeBlock(const Terrain &terrain);
    bool validBlock() {
        return breakBlock;
    }

    bool getWaterState() {
        return caminWater;
    }

    bool getLavaState() {
        return caminLava;
    }
    // For sending the Player's data to the GUI
    // for display
    QString posAsQString() const;
    QString velAsQString() const;
    QString accAsQString() const;
    QString lookAsQString() const;
};
