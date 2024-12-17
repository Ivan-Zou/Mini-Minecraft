#include "texture.h"
#include <QImage>
#include <iostream>

Texture::Texture(OpenGLContext *context)
    : glContext(context), m_textureHandle(-1), textureImage(nullptr)
{}

Texture::~Texture()
{
    destroy();
}

void Texture::create(const char *texturePath) {
    glContext->printGLErrorLog();

    QImage img(texturePath);
    if (img.isNull()) std::cerr << "Failed to load texture at path: " << texturePath << std::endl;
    img.convertTo(QImage::Format_ARGB32);
    img = img.mirrored();
    textureImage = std::make_unique<QImage>(img);
    glContext->glGenTextures(1, &m_textureHandle);

    glContext->printGLErrorLog();
}

void Texture::load(int texSlot = 0) {
    glContext->printGLErrorLog();

    glContext->glActiveTexture(GL_TEXTURE0 + texSlot);
    glContext->glBindTexture(GL_TEXTURE_2D, m_textureHandle);

    // Set the image filtering and UV wrapping options for the texture.
    // These parameters need to be set for EVERY texture you create.
    // They don't always have to be set to the values given here,
    // but they do need to be set.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glContext->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                            textureImage->width(), textureImage->height(),
                            0, GL_BGRA, GL_UNSIGNED_BYTE, textureImage->bits());

    glContext->printGLErrorLog();
}

void Texture::bind(int texSlot = 0)
{
    glContext->printGLErrorLog();
    glContext->glActiveTexture(GL_TEXTURE0 + texSlot);
    glContext->glBindTexture(GL_TEXTURE_2D, m_textureHandle);

    glContext->printGLErrorLog();
}

GLuint Texture::getHandle() const {
    return m_textureHandle;
}

void Texture::destroy() {
    glContext->glDeleteTextures(1, &m_textureHandle);
}
