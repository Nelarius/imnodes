#pragma once

#include <string>
#include <map>

struct Port {
    int id;
    std::string type;  // "INPUT" or "OUTPUT"
    char name[40] = "";
    char* reference_name = nullptr;  // used for input ports only

    Port() {};
    Port(int n, std::string s) : id(n), type(s) {}
};

struct Parameter {
    int id;
    std::string name; // ex. delay_amt
    std::string type; // ex. bool
    char value[40] = "";
    
    Parameter() {};
    Parameter(int n, std::string param_name, std::string param_type) : id(n), name(param_name), type(param_type) {}
};

class Block {
public:
    std::string name;

    std::map<int, Port> _inPorts;
    std::map<int, Port> _outPorts;

    std::map<int, Parameter> _parameters;
    
    Block() : id{ 0 }, type{"Block"} {}
    Block(const int i, std::string n) : id {i}, type {n} {}

    int getID();
    std::string getName();
    std::string getType();
    int getNumInputs();
    int getNumOutputs();

    void show();
    void setName(std::string n);
    void addInPort(int current_port_id, Port p);
    void addOutPort(int current_port_id, Port p);
    void addParam(int current_param_id, Parameter p);
    void deleteInPort(int portid);
    void deleteOutPort(int portid);
    void bypass();
    void deleteBlock(Block block);

private :
    int id;
    std::string type;
    bool is_active = true;   
};