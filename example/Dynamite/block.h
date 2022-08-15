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
    char* reference_name = nullptr;
    Port() {};
    Port(int n, string s) {
        this->id = n;
        this->type = s;
    };
};

struct Parameter {
    int id;
    string name;
    string type;
    char value[40] = "";
    Parameter() {};
    Parameter(int n, string param_name, string param_type) {
        this->id = n;
        this->name = param_name;
        this->type = param_type;
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

        map<int, Parameter> _parameters;
        
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
        void addParam(int current_param_id, Parameter p);
        void deleteInPort(int portid);
        void deleteOutPort(int portid);
        void bypass();
        void deleteBlock(Block block);
};