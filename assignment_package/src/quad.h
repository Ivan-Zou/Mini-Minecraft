#pragma once

#include "drawable.h"

class Quad : public Drawable {
public:
    Quad(OpenGLContext* context);
    void createVBOdata() override;
};
