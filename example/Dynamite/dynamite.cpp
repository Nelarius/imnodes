#include "dynamite.h"

Dynamite::Dynamite() { } 

void Dynamite::init() {
    m_ui.init();
    m_context.init();
}

bool Dynamite::show(bool done) {
    // loadContext, return a context or smth 
    // then go display context 
    return done = m_ui.show(done, m_context); 
}

void Dynamite::exit() {
    m_ui.exit();
}



