#include "dynamite.h"

DSPDynamite::DSPDynamite() {
    // saved context state loaded from metadata
    context = ImNodes::EditorContextCreate();
}

void DSPDynamite::init() {
    ui.init();
}

void DSPDynamite::show(bool done) {
    ui.show(context, done); 
}

void DSPDynamite::exit() {
    ui.exit(context);
}



