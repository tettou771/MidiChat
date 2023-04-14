#include "Thumbnail.h"

ofEvent<string> Thumbnail::selectedEvents;

Thumbnail::Thumbnail(string &sequenceStr) {
    // make thumbnail
    ofFboSettings s;
    s.width = 40;
    s.height = 40;
    s.internalformat = GL_RGB;
    s.maxFilter = GL_NEAREST;
    fbo.allocate(s);
    fbo.begin();
    ofPushStyle();
    {
        ofSetColor(0, 100, 100);
        ofFill();
        ofDrawRectangle(0, 0, s.width, s.height);
        
        ofSetColor(20, 150, 150);
        ofNoFill();
        ofSetLineWidth(2);
        ofDrawRectangle(0, 0, s.width, s.height);

        ofSetColor(255);
        fbo.draw(0, 0, getWidth(), getHeight());
        ofDrawBitmapString("MIDI\nDATA", 4, (s.height - 28) / 2 + 15);
    }
    ofPopStyle();
    fbo.end();
    
    this->sequenceStr = sequenceStr;
}

void Thumbnail::onDraw() {
    if (!fbo.isAllocated()) return;

    fbo.draw(0, 0, getWidth(), getHeight());
}

void Thumbnail::onMousePressedOverComponent(ofMouseEventArgs &mouse) {
    ofNotifyEvent(selectedEvents, sequenceStr);
    ofSetClipboardString("```\n" + sequenceStr + "\n```");
}
