#include "dynamite.h"

DSPDynamite::DSPDynamite() {
    // saved context state loaded from metadata
    context = ImNodes::EditorContextCreate();
}

void DSPDynamite::init() {
    ui.init();
}

bool DSPDynamite::show(bool done) {
    return done = ui.show(context, done); 
}

void DSPDynamite::exit() {
    ui.exit(context);
}



