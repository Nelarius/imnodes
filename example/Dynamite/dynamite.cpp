#include "dynamite.h"
// #include <stdio.h>

Dynamite::Dynamite() { } 

void Dynamite::init() {
    m_ui.init();
    m_context.init();
}

bool Dynamite::show(bool done) {
    return done = m_ui.show(done, m_context); 
}

void Dynamite::exit() {
    m_ui.exit();
    JsonGraphFileWriter fw;
    fw.writeToFile(m_context);
}



