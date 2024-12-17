#pragma once
#include "openglcontext.h"
#include <memory>
#include <QImage>

class Texture
{
private:
    OpenGLContext* glContext;
    GLuint m_textureHandle;
    std::unique_ptr<QImage> textureImage;

public:
    Texture(OpenGLContext* context);
    ~Texture();

    void create(const char *texturePath);
    void load(int texSlot);
    void bind(int texSlot);
    void destroy();

    GLuint getHandle() const;

};
