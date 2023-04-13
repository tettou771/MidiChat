#include "Thumbnail.h"

ofEvent<string> Thumbnail::selectedEvents;

Thumbnail::Thumbnail(string &sequenceStr) {
    // make thumbnail
    ofFboSettings s;
    s.width = 100;
    s.height = 100;
    s.maxFilter = GL_NEAREST;
    fbo.allocate(s);
    fbo.begin();
    ofClear(100);
    ofSetColor(200);
    ofNoFill();
    ofDrawRectangle(0, 0, fbo.getWidth()-1, fbo.getHeight()-1);
    fbo.end();
    
    this->sequenceStr = sequenceStr;
}

void Thumbnail::onDraw() {
    if (!fbo.isAllocated()) return;
    
    ofSetColor(0, 100, 100);
    ofFill();
    ofDrawRectangle(0, 0, getWidth(), getHeight());

    ofSetColor(50, 150, 150);
    ofNoFill();
    ofDrawRectangle(0, 0, getWidth(), getHeight());

    ofSetColor(255);
    fbo.draw(0, 0, getWidth(), getHeight());
    ofDrawBitmapString("MIDI\nDATA", 4, (getHeight() - 28) / 2 + 15);
}

void Thumbnail::onMousePressedOverComponent(ofMouseEventArgs &mouse) {
    ofNotifyEvent(selectedEvents, sequenceStr);
}
