#include "WatchFace.h"

WatchFace::WatchFace(Arduino_GFX *gfx) {
    _gfx = gfx;
}

void WatchFace::draw(bool fullUpdate) {
    if(fullUpdate) {
        _gfx->fillScreen(BLACK);
        _gfx->setTextSize(6); _gfx->setTextColor(WHITE);
        _gfx->setCursor(100, 150); _gfx->print("12:00");
        
        // Icon
        _gfx->fillCircle(184, 350, 50, BLUE);
        _gfx->drawCircle(184, 350, 50, WHITE);
        _gfx->setTextSize(3); _gfx->setTextColor(WHITE);
        _gfx->setCursor(168, 340); _gfx->print("GO");
    }
}