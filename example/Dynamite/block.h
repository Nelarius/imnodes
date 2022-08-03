#pragma once

#include <imgui.h>
#include <imnodes.h>

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <iterator>

using namespace std;

class Block {

    int id;
    string name;
    bool is_active = true;   

    public:

        map<int, string> _inPorts;
        map<int, string> _outPorts;
        
        Block();
        Block(const int i);
        Block(const int i, string n);

        int getID();
        string getName();
        string getInput();
        string getOutput();
        int getNumInputs();
        int getNumOutputs();

        void show();
        void addInPort(int current_port_id, string chan_name);
        void addOutPort(int current_port_id, string chan_name);
        void deleteInPort(int portid);
        void deleteOutPort(int portid);
        void bypass();
        void deleteBlock(Block block);
};