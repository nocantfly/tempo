#ifndef WATCHFACE_H
#define WATCHFACE_H
#include "config.h"
#include <Arduino_GFX_Library.h>

class WatchFace {
public:
    WatchFace(Arduino_GFX *gfx);
    void draw(bool fullUpdate = false);
private:
    Arduino_GFX *_gfx;
};
#endif