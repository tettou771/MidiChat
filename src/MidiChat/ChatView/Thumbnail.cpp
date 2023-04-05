#include "Thumbnail.h"

ofEvent<ofJson> Thumbnail::selectedEvents;

Thumbnail::Thumbnail(ofJson &j) {
    // make thumbnail
    ofFboSettings s;
    s.width = 100;
    s.height = 100;
    s.maxFilter = GL_NEAREST;
    fbo.allocate(s);
    fbo.begin();
    ofClear(100);
    fbo.end();
}

void Thumbnail::onDraw() {
    if (!fbo.isAllocated()) return;
    fbo.draw(0, 0, getWidth(), getHeight());
}

void Thumbnail::onMousePressedOverComponent(ofMouseEventArgs &mouse) {
    ofNotifyEvent(selectedEvents, json);
}
