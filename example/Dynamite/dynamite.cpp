#include "dynamite.h"

Dynamite::Dynamite() {
    // saved context state loaded from metadata
    m_context = ImNodes::EditorContextCreate();
    ImNodes::EditorContextSet(m_context);
} 

void Dynamite::init() {
    m_ui.init();
}

bool Dynamite::show(bool done) {
    return done = m_ui.show(done); 
}

void Dynamite::exit() {
    m_ui.exit(m_context);
}



