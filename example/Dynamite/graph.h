#include <list>

#include "block.h"

struct adjlist_node {
    int dest;
    struct adjlist_node* next;
};

struct adjlist {
    struct adjlist_node* head;
};

class Graph {

    private:
        int num_vertices;
        struct adjlist* array;

    public: 
        Graph();
        Graph(int v) {
            this->num_vertices = v;
            array = new adjlist[v];
            for (int i = 0; i<v; i++) {
                array[i].head = NULL;
            }
        }
        void init(int num_vertices);
        adjlist_node* new_node(int dest);
        void add_edge(int src, int dest);
        void display();
        bool contains_edge(int src, int dest);

};