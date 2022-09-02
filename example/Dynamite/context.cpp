#include "context.h"
#include "dyndsp_wrapper.h"
#include "JsonGraphFileWriter.h"
#include <SDL_scancode.h>
#include <iostream>

Context::Context() {
}

void Context::init() {
    m_graph.init();
}
void Context::loadContext() { }

void Context::sortGraph() {
    m_graph.buildAdjacencyList();
    m_graph.topologicalSort(); // sorts adjacency list
}


