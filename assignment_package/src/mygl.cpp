#include "mygl.h"
#include "blockworker.h"
#include "vboworker.h"
#include <glm_includes.h>

#include <QApplication>
#include <QKeyEvent>
#include <QDateTime>
#include <QThreadPool>

#include "framebuffer.h"

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_worldAxes(this),
      m_progLambert(this), m_progFlat(this), m_progInstanced(this),
      m_terrain(this), m_player(glm::vec3(-91.f, 271.f, 103.f), m_terrain),
      m_texture(this), animateTime(0), quad(this), m_progSky(this),  m_progFluid(this),
      postProcessFrameBuffer(this, this->width(), this->height(), this->devicePixelRatio())
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Enable Transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    m_worldAxes.createVBOdata();

    // Create and set up the diffuse shader
    m_progLambert.create(":/glsl/lambert.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    // Create and set up the sky shader
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");

    // Create and set up fluid shader
    m_progFluid.create(":/glsl/fluid.vert.glsl", ":/glsl/fluid.frag.glsl");

    quad.createVBOdata();

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);
    // Bind the minecraft texture
    m_texture.create(":/textures/minecraft_textures_all.png");
    m_texture.load(0);
    m_progLambert.setUnifInt("u_Texture", 0);
    //m_progFluid.setUnifInt("u_Texture", 0);

    postProcessFrameBuffer.resize(this->width(), this->height(), this->devicePixelRatio());
    postProcessFrameBuffer.create();
}


void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progLambert.setUnifMat4("u_ViewProj", viewproj);
    m_progFlat.setUnifMat4("u_ViewProj", viewproj);
    m_progInstanced.setUnifMat4("u_ViewProj", viewproj);


    postProcessFrameBuffer.resize(w, h, this->devicePixelRatio());
    postProcessFrameBuffer.create();

    printGLErrorLog();


}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    double currTime = QDateTime::currentMSecsSinceEpoch();
    double deltaTime = currTime - lastTime;
    lastTime = currTime;
    m_player.tick(deltaTime, m_inputs);
    glm::vec3 currPos = m_player.mcr_position;
    expand(prevPos, currPos);
    ++animateTime;
    m_progLambert.setUnifInt("u_Time", animateTime);
    m_progSky.setUnifInt("u_Time", animateTime);
    m_progFluid.setUnifInt("u_Time", animateTime);

    m_progLambert.setUnifVec3("u_CamPos", glm::vec3(m_player.mcr_position.x, m_player.mcr_position.y, m_player.mcr_position.z));

    update(); // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
    prevPos = currPos;
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    m_texture.bind(0);
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();
    m_progLambert.setUnifMat4("u_ViewProj", viewproj);
    m_progLambert.setUnifMat4("u_Model", glm::mat4());
    m_progLambert.setUnifMat4("u_ModelInvTr", glm::mat4());
    m_progFlat.setUnifMat4("u_ViewProj", viewproj);
    m_progInstanced.setUnifMat4("u_ViewProj", viewproj);


    //renderTerrain();
    render3dScene();

    glDisable(GL_DEPTH_TEST);
    m_progFlat.setUnifMat4("u_Model", glm::mat4());
    // m_progFlat.draw(m_worldAxes);
    glEnable(GL_DEPTH_TEST);

    // Set values for ray casting in the sky
    m_progSky.setUnifVec3("eye", m_player.mcr_camera.mcr_position);
    m_progSky.setUnifVec3("R", m_player.mcr_camera.R());
    m_progSky.setUnifVec3("U", m_player.mcr_camera.U());
    m_progSky.setUnifVec3("F", m_player.mcr_camera.F());
    m_progSky.setUnifFloat("aspect", this->width() / (float) this->height());

    m_progLambert.setUnifVec3("eye", m_player.mcr_camera.mcr_position);
    m_progLambert.setUnifVec3("R", m_player.mcr_camera.R());
    m_progLambert.setUnifVec3("U", m_player.mcr_camera.U());
    m_progLambert.setUnifVec3("F", m_player.mcr_camera.F());
    m_progLambert.setUnifFloat("aspect", this->width() / (float) this->height());


    m_progFluid.setUnifInt("inWater", m_player.getWaterState() ? 1 : 0);
    m_progFluid.setUnifInt("inLava", m_player.getLavaState() ? 1 : 0);

    //m_progLambert.setUnifInt("switchBiome", m_player.currBiome);
    // m_progLambert.setUnifInt("inLava", m_player.getLavaState() ? 1 : 0);


    // Draw the sky
    m_progSky.draw(quad);

    //Post Process
    renderPostProcess();

}


void MyGL::expand(glm::vec3 prevPos, glm::vec3 currPos) {
    // Compute zone coordinates
    glm::ivec2 prevZone = glm::ivec2(floor(prevPos.x / 64.f) * 64, floor(prevPos.z / 64.f) * 64);
    glm::ivec2 currZone = glm::ivec2(floor(currPos.x / 64.f) * 64, floor(currPos.z / 64.f) * 64);
    // If currZone does not exist in terrain and zones have changed, expand.
    if (prevZone != currZone) {
        for (int dx = -10; dx < 10; ++dx) {
            for (int dz = -10; dz <= 10; ++dz) {
                glm::ivec2 curr = currZone + (glm::ivec2(dx, dz) * 16);
                if (!m_terrain.hasChunkAt(curr.x, curr.y)) {
                    Chunk* cPtr = m_terrain.instantiateChunkAt(curr.x, curr.y);
                    blockMutex.lock();
                    for (Chunk* n : cPtr->getNeighbors()) {
                        vboData.insert(n);
                    }
                    blockMutex.unlock();
                    BlockWorker *bw = new BlockWorker(&m_terrain, &blockMutex, curr.x, curr.y, &vboData);
                    QThreadPool::globalInstance()->start(bw);
                } else {
                    Chunk* cPtr = m_terrain.getChunkAt(curr.x, curr.y).get();
                    if (cPtr->elemCount(INDEX) < 0) {
                        VBOWorker *vw = new VBOWorker(cPtr, &vboMutex, &needBinding);
                        QThreadPool::globalInstance()->start(vw);
                    }
                }
            }
        }
    }
    // Check shared data structures
    blockMutex.lock();
    for (Chunk* cPtr : vboData) {
        VBOWorker *vw = new VBOWorker(cPtr, &vboMutex, &needBinding);
        QThreadPool::globalInstance()->start(vw);
    }
    vboData.clear();
    blockMutex.unlock();
    // Bind to GPU
    vboMutex.lock();
    for (Chunk* cPtr : needBinding) {
        cPtr->createVBOdata();
    }
    needBinding.clear();
    vboMutex.unlock();
}


// Change this so it renders the nine zones of generated
// terrain that surround the player (refer to Terrain::m_generatedTerrain
// for more info)
void MyGL::renderTerrain() {

    m_texture.bind(0);
    int x = glm::floor(m_player.mcr_position.x / 16.f) * 16;
    int z = glm::floor(m_player.mcr_position.z / 16.f) * 16;
    m_terrain.draw(x - 3 * 64, x + 3 * 64, z - 3 * 64, z + 3 * 64, &m_progLambert);
}

// Bind the post-process frame buffer,
// diverting the output of  render
// calls to the texture within the frame buffer
// rather than MainWindow display.
void MyGL::render3dScene() {
    postProcessFrameBuffer.bindFrameBuffer();
    glViewport(0, 0, this->width(), this->height());
    // Clear the screen so that we only see newly drawn images
    glClearColor(0.37f, 0.74f, 1.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderTerrain();

}

//Renders post process effects, player in fluid
void MyGL::renderPostProcess() {
    // Bind the default framebuffer (screen framebuffer)
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // Set the viewport to match the window size
   glViewport(0, 0, this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());

    // Clear the screen with a solid color
    glClearColor(0.37f, 0.74f, 1.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    printGLErrorLog();

    // Bind the texture from the post-processing framebuffer to texture slot 0
    postProcessFrameBuffer.bindToTextureSlot(1);
    printGLErrorLog();

    // Use the post-processing shader program
    m_progFluid.useMe();

    // Set the texture uniform for the post-processing shader
    m_progFluid.setUnifInt("u_Texture", postProcessFrameBuffer.getTextureSlot());

    // Draw the full-screen quad with the texture
    m_progFluid.draw(quad);

    // Additional debugging
    printGLErrorLog();
}


void MyGL::keyPressEvent(QKeyEvent *e) {
    float amount = 2.0f;

    if(e->modifiers() & Qt::ShiftModifier){
        amount = 10.0f;
    }
    // http://doc.qt.io/qt-5/qt.html#Key-enum
    // This could all be much more efficient if a switch
    // statement were used, but I really dislike their
    // syntax so I chose to be lazy and use a long
    // chain of if statements instead
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
    } else if (e->key() == Qt::Key_Right) {
        m_player.rotateOnUpGlobal(-amount);
    } else if (e->key() == Qt::Key_Left) {
        m_player.rotateOnUpGlobal(amount);
    } else if (e->key() == Qt::Key_Up) {
        m_player.rotateOnRightLocal(-amount);
    } else if (e->key() == Qt::Key_Down) {
        m_player.rotateOnRightLocal(amount);
    } else if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = true;
        //m_player.moveForwardLocal(amount);
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = true;
        //m_player.moveForwardLocal(-amount);
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = true;
        //m_player.moveRightLocal(amount);
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = true;
        // m_player.moveRightLocal(-amount);
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = true;
        //m_player.moveUpGlobal(-amount);
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = true;
        //m_player.moveUpGlobal(amount);
    } else if (e->key() == Qt::Key_F) {
        m_player.toggleFlightMode();
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = true;
    }



}

void MyGL::keyReleaseEvent(QKeyEvent *e) {

    if (e->key() == Qt::Key_W) {
        m_inputs.wPressed = false;
        //m_player.moveForwardLocal(amount);
    } else if (e->key() == Qt::Key_S) {
        m_inputs.sPressed = false;
        //m_player.moveForwardLocal(-amount);
    } else if (e->key() == Qt::Key_D) {
        m_inputs.dPressed = false;
        //m_player.moveRightLocal(amount);
    } else if (e->key() == Qt::Key_A) {
        m_inputs.aPressed = false;
        // m_player.moveRightLocal(-amount);
    } else if (e->key() == Qt::Key_E) {
        m_inputs.ePressed = false;
    } else if (e->key() == Qt::Key_Q) {
        m_inputs.qPressed = false;
    } else if (e->key() == Qt::Key_Space) {
        m_inputs.spacePressed = false;
    }

}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    glm::vec2 pos(e->pos().x(), e->pos().y());

    //Game Mode
    if (pos.x == 0 && pos.y == 0) {
        return;
    }

    float screenX = this->width() / 2.0;
    float screenY = this->height() / 2.0;

    float rotateUp = 0.09 * (screenX - pos.x);
    float rotateRight = 0.09 * (screenY - pos.y);
    m_player.rotateOnUpGlobal(rotateUp);
    m_player.rotateOnRightLocal(rotateRight);
    moveMouseToCenter();

}

void MyGL::mousePressEvent(QMouseEvent *e) {

    if(e->buttons() & (Qt::RightButton))
    {
        glm::vec3 blockCoord = m_player.placeBlock(m_terrain);

       // qDebug() << blockCoord.x << " " << blockCoord.y << " " << blockCoord.z;
        m_terrain.setGlobalBlockAt(blockCoord.x, blockCoord.y, blockCoord.z, WOOD);
       // m_terrain.generateTree(blockCoord.x, blockCoord.y, blockCoord.z);
    }

    if(e->buttons() & (Qt::LeftButton))
    {
        glm::vec3 blockCoord = m_player.getBlock(m_terrain);
        if (m_player.validBlock()) {
            m_terrain.setGlobalBlockAt(blockCoord.x, blockCoord.y, blockCoord.z, EMPTY);
        }

    }


}
