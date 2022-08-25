#include "graph.h"

#include <iostream>

using namespace std;

struct adjlist_node;
struct adjlist;

Graph::Graph() {
}

void Graph::init(int num_vertices) {
    // when num_vertices is grabbed as _blocks.size()
    this->num_vertices = num_vertices;
}

adjlist_node* Graph::new_node(int dest) {
    adjlist_node* newNode = new adjlist_node;
    newNode->dest = dest;
    newNode->next = NULL;
    return newNode;
}

void Graph::add_edge(int src, int dest) {
    adjlist_node* newNode = new_node(dest);
    newNode->next = array[src].head;
    array[src].head = newNode;
}

void Graph::display() {
    int v;
    for (v = 0; v < num_vertices; ++v)
    {
        adjlist_node* pCrawl = array[v].head;
        cout<<"\n Adjacency list of vertex "<<v<<"\n head ";
        while (pCrawl)
        {
            cout<<"-> "<<pCrawl->dest;
            pCrawl = pCrawl->next;
        }
        cout<<endl;
    }
}

bool Graph::contains_edge(int src, int dest) {
    for (adjlist_node* node = array[src].head; node; node = node->next) {
        if (node->dest == dest) {
            return true;
        }
    }
    return false;
}