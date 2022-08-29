#include "context.h"
#include "dyndsp_wrapper.h"
#include <SDL_scancode.h>
#include <iostream>

Context::Context() {
}

/*Context::~Context() {
    ImNodes::EditorContextFree(m_context);
}*/

void Context::init() {
    m_graph.init();
}
void Context::loadContext() { }

void Context::buildGraph() {
    m_graph.buildAdjacencyList();
    m_graph.display();
    m_graph.topologicalSort();
}


