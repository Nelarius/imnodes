#pragma once

#include <imgui.h>
#include <imnodes.h>

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <iterator>

using namespace std;

struct Port {
    int id;
    string type;
    char name[40] = "";
    Port() {};
    Port(int n, string s) {
        this->id = n;
        this->type = s;
    };
};

class Block {

    int id;
    string type;
    bool is_active = true;   

    public:
        string name;

        map<int, Port> _inPorts;
        map<int, Port> _outPorts;
        
        Block();
        Block(const int i);
        Block(const int i, string n);

        int getID();
        string getName();
        string getType();
        string getInput();
        string getOutput();
        int getNumInputs();
        int getNumOutputs();

        void show();
        void setName(string n);
        void addInPort(int current_port_id, Port p);
        void addOutPort(int current_port_id, Port p);
        void deleteInPort(int portid);
        void deleteOutPort(int portid);
        void bypass();
        void deleteBlock(Block block);
};