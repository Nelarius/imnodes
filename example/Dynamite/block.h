#pragma once

#include <imgui.h>
#include <imnodes.h>

#include <algorithm>
#include <vector>
#include <string>

using namespace std;

class Block {

    int id;
    int numInCh;
    int numOutCh;
    string name;
    string input;
    string output;
    bool is_active = true;

    public:
        
        Block();
        Block(const int i);
        Block(const int i, string n) : id(i), name(n) {}
        Block(const int i, string n, string in, string o) : id(i), name(n), input(in), output(o) {}

        int getID();
        string getName();
        string getInput();
        string getOutput();
        void show();
        void bypass();
        void deleteBlock(Block block);
        
};