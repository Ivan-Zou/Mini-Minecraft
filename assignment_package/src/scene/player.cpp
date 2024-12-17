#include "player.h"
#include <QString>
#include <iostream>
#include <QDebug>
#include <QSoundEffect>
#include <QDir>

Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
}

void Player::processInputs(InputBundle &inputs) {
    // Updates the Player's velocity and acceleration based on the
    // state of the inputs.

    // Reset acceleration for this frame
    m_acceleration = glm::vec3(0);

    if (!flightMode) {
        float dampen = 1.f;
        if (onWater || onLava) {
            onGround = false;
            dampen = 0.3;
            m_acceleration.y = -0.001;
        } else {
            m_acceleration.y = -0.0039;
        }

        // qDebug() <<"here";
        if ((onWater || onLava) && inputs.spacePressed) {
            //qDebug() << "SWIM";
            m_acceleration.y += 0.002;
        } else if (onGround && inputs.spacePressed) {
            m_velocity.y = 0.05;
            if (inputs.wPressed) {
                m_velocity.z = 0.018;
            }
            onGround = false;
        }

        // Check for input to adjust acceleration along the Z axis (forward/backward movement)
        if (inputs.wPressed == inputs.sPressed) {
            m_acceleration.z = 0; // No forward/backward movement
        } else if (inputs.wPressed) {
            m_acceleration.z = 0.0018f * dampen; // Moving forward
        } else if (inputs.sPressed) {
            m_acceleration.z = -0.0018f * dampen; // Moving backward
        }

        // Check for input to adjust acceleration along the X axis (left/right movement)
        if (inputs.aPressed == inputs.dPressed) {
            m_acceleration.x = 0; // No left/right movement
        } else if (inputs.aPressed) {
            m_acceleration.x = -0.0018f * dampen; // Moving left
        } else if (inputs.dPressed) {
            m_acceleration.x = 0.0018f * dampen; // Moving right
        }

        if (inputs.aPressed || inputs.dPressed || inputs.wPressed || inputs.sPressed || inputs.spacePressed) {
            playerMove = true;
        } else {
            playerMove = false;
        }



    } else {
        // Check for input to adjust acceleration along the Y axis (up/down movement)
        if (inputs.ePressed == inputs.qPressed) {
            m_acceleration.y = 0;
        } else if (inputs.qPressed) {
            m_acceleration.y = -0.005f; // Moving down
        } else if (inputs.ePressed) {
            m_acceleration.y = 0.005f; // Moving Up
        }

        // Check for input to adjust acceleration along the Z axis (forward/backward movement)
        if (inputs.wPressed == inputs.sPressed) {
            m_acceleration.z = 0; // No forward/backward movement
        } else if (inputs.wPressed) {
            m_acceleration.z = 0.008f; // Moving forward
        } else if (inputs.sPressed) {
            m_acceleration.z = -0.008f; // Moving backward
        }

        // Check for input to adjust acceleration along the X axis (left/right movement)
        if (inputs.aPressed == inputs.dPressed) {
            m_acceleration.x = 0; // No left/right movement
        } else if (inputs.aPressed) {
            m_acceleration.x = -0.008f; // Moving left
        } else if (inputs.dPressed) {
            m_acceleration.x = 0.008f; // Moving right
        }
    }


    // Update velocity with acceleration, clamping to max speed
    m_velocity.x = glm::clamp(m_velocity.x + m_acceleration.x, -maxVel, maxVel);
    m_velocity.z = glm::clamp(m_velocity.z + m_acceleration.z, -maxVel, maxVel);  // For forward/backward (Z-axis)
    m_velocity.y = glm::clamp(m_velocity.y + m_acceleration.y, -maxVel, maxVel);

}

glm::vec3 Player::normalizeVelocity() {
    glm::vec3 normalF = m_forward;
    normalF.y = 0;
    normalF = glm::normalize(normalF);

    glm::vec3 normalR = m_right;
    normalR.y = 0;
    normalR = glm::normalize(normalR);

    return (normalF * m_velocity.z) + (normalR * m_velocity.x);
}

const std::array<float, 2> player_bb_x {
    -0.48, 0.48
};
const std::array<float, 3> player_bb_y {
    0, 0.9, 1.9
};
const std::array<float, 2> player_bb_z {
    -0.48, 0.48
};

void Player::computePhysics(float dT, const Terrain &terrain) {
    // TODO: Update the Player's position based on its acceleration
    // and velocity, and also perform collision detection.
    if (flightMode) {
     moveRightLocal(m_velocity.x * dT);
     moveForwardLocal(m_velocity.z * dT);
     moveUpGlobal(m_velocity.y * dT);
     onGround = false;
    } else {


        glm::vec3 normalVel = normalizeVelocity();
        bool collisionX = false, collisionZ = false, collisionY = false;
        float minDisX = 100000.f;
        glm::ivec3 closeBlockX(0, 0, 0);
        float minDisZ = 100000.f;
        glm::ivec3 closeBlockZ(0, 0, 0);
        float minDisY = 100000.f;
        glm::ivec3 closeBlockY(0, 0, 0);

        // Collision checking
        for (float y : player_bb_y) {
            for (float x : player_bb_x) {
                for (float z : player_bb_z) {
                    glm::vec3 rayPos = glm::vec3(m_position.x + x, m_position.y + y, m_position.z + z);
                    float dis = 0;
                    glm::ivec3 outBlock;

                    //Will perform grid march on all axis
                    glm::vec3 rayDirX = glm::vec3(normalVel.x,  0, 0) * dT * 16.5f;
                    glm::vec3 rayDirY = glm::vec3(0,  m_velocity.y, 0) * dT * 15.f;
                    glm::vec3 rayDirZ = glm::vec3(0,  0, normalVel.z) * dT * 16.5f;

                    if (gridMarch(rayPos, rayDirX, terrain, &dis, &outBlock)) {
                        collisionX = true;
                        if (dis < minDisX) {
                            minDisX = dis;
                            closeBlockX = outBlock;
                        }
                    }

                    //Could be cause bottom left corner
                    if (gridMarch(rayPos, rayDirZ, terrain, &dis, &outBlock)) {
                        collisionZ = true;
                        if (dis < minDisZ) {
                            minDisZ = dis;
                            closeBlockZ = outBlock;
                        }
                    }

                    if (gridMarch(rayPos, rayDirY, terrain, &dis, &outBlock)) {
                        collisionY = true;
                        if (dis < minDisY) {
                            minDisY = dis;
                            closeBlockY = outBlock;
                        }
                    }
                }
            }
        }


        float displacementX = normalVel.x * dT;

        float displacementY = m_velocity.y * dT;

        float displacementZ = normalVel.z * dT;

        float offset = 0.01;
        BlockType blockY = terrain.getGlobalBlockAt(closeBlockY.x,closeBlockY.y, closeBlockY.z);
        BlockType blockX = terrain.getGlobalBlockAt(closeBlockX.x,closeBlockX.y, closeBlockX.z);
        BlockType blockZ = terrain.getGlobalBlockAt(closeBlockZ.x,closeBlockZ.y, closeBlockZ.z);


        if (playerMove && (collisionY && blockY == WATER) ||
            (collisionX && blockX == WATER) ||
            (collisionZ && blockZ == WATER)) {
            // Update collisions only if the player moved
            onWater = true;
        } else if (playerMove && m_position.y >= 139 || (playerMove && m_position.y < 135)){
            onWater = false;
        }


        if (playerMove && (collisionY && blockY == LAVA) ||
            (collisionX && blockX == LAVA) ||
            (collisionZ && blockZ == LAVA)) {
            onLava = true;
        } else if (playerMove && m_position.y >= 26){
            onLava = false;
        }

        if (onLava) {
            caminLava = (m_position.y + 1.5 < 26);
        } else {
            caminLava = false;
        }

        if (onWater) {
            caminWater = (m_position.y + 1.5 < 139);
        } else {
            caminWater = false;
        }



        if (displacementZ < 0 && blockZ != WATER && blockY != LAVA) {
            if (minDisZ <= offset) {
                displacementZ = 0;
            } else if (std::abs(displacementZ) > minDisZ) {
                displacementZ = -minDisZ * 0.8;
            }
        } else if (displacementZ > 0 && blockZ != WATER && blockY != LAVA) {
            if (minDisZ <= offset) {
                displacementZ = 0;
            } else if (std::abs(displacementZ) > minDisZ) {
                displacementZ = (minDisZ ) * 0.8;
            }
        }


        if (displacementX < 0 && blockX != WATER && blockY != LAVA) {
            if (minDisX <= offset) {
                displacementX = 0;
            } else if (std::abs(displacementX) > minDisX) {
                displacementX = -minDisX * 0.8;
            }
        } else if (displacementX > 0 && blockX != WATER && blockY != LAVA) {
            if (minDisX <= offset) {
                displacementX = 0;
            } else if (std::abs(displacementX) > minDisX) {
                displacementX = (minDisX) * 0.8;;
            }
        }


        if (displacementY < 0 && blockY != WATER && blockY != LAVA) {
            if (minDisY <= offset) {
                displacementY = 0;
                if (!onWater || !onLava) {
                    onGround = true;
                }
            } else if (std::abs(displacementY) > minDisY) {
                displacementY = -minDisY * 0.8;
                if (!onWater || !onLava) {
                    onGround = true;
                }
            }
        } else if (displacementY > 0 && blockY != WATER) {
            // displacementZ = (minDisZ * 0.8);
            if (minDisY <= offset) {
                displacementY = 0;
            } else if (std::abs(displacementY) > minDisY) {
                displacementY = (minDisY ) * 0.8;;
            }           
        }


        glm::vec3 adjustedDis = glm::vec3(displacementX, displacementY, displacementZ);

        // Apply movement
        moveAlongVector(adjustedDis);

    }

    // Friction Apply damping to reduce velocity gradually
    float dampingFactor = 0.7f;
    m_velocity.x *= dampingFactor;
    m_velocity.z *= dampingFactor;
    m_velocity.y *= dampingFactor;

    // Ensure very small velocities are set to zero to avoid oscillations
    if (glm::abs(m_velocity.x) < 0.001f) {
        m_velocity.x = 0;
    }
    if (glm::abs(m_velocity.z) < 0.001f) {
        m_velocity.z = 0;
    }
    if (glm::abs(m_velocity.y) < 0.001f) {
        m_velocity.y = 0;
    }


}

//From Lecture
bool Player::gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getGlobalBlockAt(currCell.x, currCell.y, currCell.z);
        if (maxLen > curr_t) {
            if(cellType != EMPTY) {
                *out_blockHit = currCell;
                *out_dist = glm::min(maxLen, curr_t);
                return true;
            }
        }

    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

glm::vec3 Player::getBlock(const Terrain &terrain) {

    float minDisF = 100000.f;
    glm::ivec3 closeBlockF;
    bool collision = false;

    glm::vec3 rayPos = glm::vec3(m_position.x, m_position.y + 1.5, m_position.z);
    glm::vec3 rayDir = glm::normalize(m_forward) * 3.0f;
    float dis = 0;
    glm::ivec3 outBlock;


    if (gridMarch(rayPos, rayDir, terrain, &dis, &outBlock)) {
        collision= true;
        minDisF = dis;
        closeBlockF = outBlock;
    }

    BlockType block = terrain.getGlobalBlockAt(closeBlockF.x,closeBlockF.y, closeBlockF.z);
    if (collision) {
        if (block == EMPTY || block == WATER || block == LAVA || block == BEDROCK) {
            breakBlock = false;
            return glm::vec3(-1, -1, -1);
        }
        breakBlock = true;
        return glm::vec3(closeBlockF.x, closeBlockF.y, closeBlockF.z);
    }

    breakBlock = false;
    return glm::vec3(-1, -1, -1);
}

glm::vec3 GetCubeNormal(const glm::vec3& P)
{
    int idx = 0;
    float val = -1;
    for(int i = 0; i < 3; i++){
        if(glm::abs(P[i]) > val){
            idx = i;
            val = glm::abs(P[i]);
        }
    }
    glm::vec3 N(0,0,0);
    N[idx] = glm::sign(P[idx]);
    return N;
}


glm::vec3 Player::placeBlock(const Terrain &terrain) {
    float minDisF = 100000.f;
    glm::ivec3 closeBlockF;
    bool collision = false;

    glm::vec3 rayPos = glm::vec3(m_position.x, m_position.y + 1.5, m_position.z);
    glm::vec3 rayDir = glm::normalize(m_forward) * 3.0f;
    float dis = 0;
    glm::ivec3 outBlock;


    if (gridMarch(rayPos, rayDir, terrain, &dis, &outBlock)) {
        collision= true;
        minDisF = dis;
        closeBlockF = outBlock;
    }

    if (collision) {
        glm::vec3 look = m_forward;
        look.y = look.y * 1.75f;
        glm::vec3 fnorm = -1.f * (GetCubeNormal((glm::normalize(look) * 3.f)));

        glm::vec3 placement = glm::vec3(closeBlockF.x, closeBlockF.y, closeBlockF.z) + (fnorm);
        BlockType block = terrain.getGlobalBlockAt(placement.x, placement.y, placement.z);
        if (block != EMPTY || block == WATER || block == LAVA || block == BEDROCK) {
            return glm::vec3(-1, -1, -1);
        } else {
            return placement;
        }

    }

    return glm::vec3(-1, -1, -1);
}


void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}
