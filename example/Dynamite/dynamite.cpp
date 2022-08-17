#include "dynamite.h"

Dynamite::Dynamite() { } 

void Dynamite::init() {
    m_ui.init();
    m_context.init();
    m_wrapper.getData(m_context);
}

bool Dynamite::show(bool done) {
    return done = m_ui.show(done, m_context, m_wrapper); 
}

void Dynamite::exit() {
    m_ui.exit();
}



