#include "dyndsp_wrapper.h" 

#include "pyhelper.h"
#include <fstream>
#include <stdio.h>

// forward declarations
bool validateIP(std::string ip);
bool isNumber(const std::string &str);
vector<string> split(const string &str, char delim);

CPyInstance hInstance; // Single initialization of Python interpreter

void DyndspWrapper::call_dyndsp_command(std::string command) {
    // Provide path for Python to find file
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\"./example/Dynamite\")");

    // Find a Python file named run_test.py
    CPyObject pName = PyUnicode_FromString("run_test");
    CPyObject pModule = PyImport_Import(pName);

    if (pModule) 
    {
        CPyObject pCommand = PyObject_GetAttrString(pModule, command.c_str()); // Find the method defined in Python file
        if (pCommand && PyCallable_Check(pCommand)) 
        {
            if ((strcmp(command.c_str(), "deploy") == 0) || (strcmp(command.c_str(), "clean") == 0)) 
            {
                // validate IP
                if (validateIP(*ip_address)) {
                    CPyObject pArg = PyTuple_New(1); // Create argument to send over
                    PyTuple_SetItem(pArg, 0, PyUnicode_FromString((*ip_address).c_str()));
                    CPyObject pResult = PyObject_CallObject(pCommand, pArg); // call object with arg
                } 
                else { printf("ERROR : cannot deploy without valid IP address\n"); }
            } 
            else { CPyObject pResult = PyObject_CallObject(pCommand, NULL); }   // call object without arg
        }
        else { printf("ERROR: function %s()\n", command.c_str()); }
    } 
    else { printf("ERROR: Module not imported\n"); }
}

// TO DO : clean up/ generalize other functions
std::vector<std::string> DyndspWrapper::get_dsp_list() 
{
    static std::vector<std::string> dsp_blocknames;

    // Provide path for Python to find file
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\"./example/Dynamite\")");
    PyRun_SimpleString("print(sys.path)");

    // Find a Python file named run_test.py
    CPyObject pName = PyUnicode_FromString("run_test");
    CPyObject pModule = PyImport_Import(pName);

    if (pModule)
    {
        // Find the method defined in Python file
	CPyObject pDSP = PyObject_GetAttrString(pModule, "list_dsp");

	if (pDSP && PyCallable_Check(pDSP))
	{
	    // Retrieve returned PyObject
	    CPyObject pListDSP = PyObject_CallObject(pDSP, NULL);
            
            auto listDSPSize = PyList_Size(pListDSP);

            // Make a string vector of dsp block names
            for (Py_ssize_t i = 0 ; i < listDSPSize; ++i)
            {
                PyObject* dsp_block = PyList_GetItem(pListDSP, i);
                std::string name(PyUnicode_AsUTF8(dsp_block));
                dsp_blocknames.push_back(name.c_str());
                // printf("C: list_dsp() = %s\n", name.c_str());
            }
	}
	else
	{
	    printf("ERROR: function list_dsp() or list_control()\n");
	}
    }
    else
    {
        printf("ERROR: Module not imported\n");
    }
    return dsp_blocknames;
}

std::vector<std::string> DyndspWrapper::get_control_list() 
{
    static std::vector<std::string> control_blocknames;

    // Provide path for Python to find file
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\"./example/Dynamite\")");
    PyRun_SimpleString("print(sys.path)");

    // Find a Python file named run_test.py
    CPyObject pName = PyUnicode_FromString("run_test");
    CPyObject pModule = PyImport_Import(pName);
    
    if (pModule)
    {
        // Find the method defined in Python file
        CPyObject pControl = PyObject_GetAttrString(pModule, "list_control");
	if (pControl && PyCallable_Check(pControl))
	{
            // Retrieve returned PyObject
            CPyObject pListControl = PyObject_CallObject(pControl, NULL);
            
            auto listControlSize = PyList_Size(pListControl);

            // Make a string vector of control block names
            for (Py_ssize_t j = 0 ; j < listControlSize; ++j)
            {
                PyObject* control_block = PyList_GetItem(pListControl, j);
                std::string name(PyUnicode_AsUTF8(control_block));
                control_blocknames.push_back(name.c_str());
                // printf("C: list_control() = %s\n", name.c_str());
            }
	}
	else
	{
	    printf("ERROR: function list_dsp() or list_control()\n");
	}
    }
    else
    {
        printf("ERROR: Module not imported\n");
    }
    return control_blocknames;
}

std::vector<std::string> DyndspWrapper::get_parameter_names(std::string block_name) 
{
    std::vector<std::string> parameters_names;

    // Find a Python file named run_test.py
    CPyObject pName = PyUnicode_FromString("run_test");
    CPyObject pModule = PyImport_Import(pName); 

    if (pModule)
    {
        // Find the method defined in Python file
        CPyObject pParams = PyObject_GetAttrString(pModule, "list_params");
	if (pParams && PyCallable_Check(pParams))
	{
            // Create argument to send over
            CPyObject pArg = PyTuple_New(1);
            PyTuple_SetItem(pArg, 0, PyUnicode_FromString(block_name.c_str()));

            // Retrieve returned PyObject
            CPyObject pListParams = PyObject_CallObject(pParams, pArg);
            
            auto listParamsSize = PyList_Size(pListParams);

            // Make a string vector of parameter names
            for (Py_ssize_t j = 0 ; j < listParamsSize; ++j)
            {
                PyObject* parameters = PyList_GetItem(pListParams, j);
                std::string name(PyUnicode_AsUTF8(parameters));
                parameters_names.push_back(name.c_str());
                // printf("C: list_params() = %s\n", name.c_str());
            }
	}
	else
	{
	    printf("ERROR: function list_params()\n");
	}
    }
    else
    {
        printf("ERROR: Module not imported\n");
    }
    return parameters_names;
}

std::vector<std::string> DyndspWrapper::get_parameter_types(std::string block_name) 
{
    std::vector<std::string> parameters_types;

    // Find a Python file named run_test.py
    CPyObject pName = PyUnicode_FromString("run_test");
    CPyObject pModule = PyImport_Import(pName);

    if (pModule)
    {
        // Find the method defined in Python file
        CPyObject pParams = PyObject_GetAttrString(pModule, "list_param_types");
	if (pParams && PyCallable_Check(pParams))
	{
            // Create argument to send over
            CPyObject pArg = PyTuple_New(1);
            PyTuple_SetItem(pArg, 0, PyUnicode_FromString(block_name.c_str()));

            // Retrieve returned PyObject
            CPyObject pListParams = PyObject_CallObject(pParams, pArg);
            
            auto listParamsSize = PyList_Size(pListParams);

            // Make a string vector of parameter names
            for (Py_ssize_t j = 0 ; j < listParamsSize; ++j)
            {
                PyObject* types = PyList_GetItem(pListParams, j);
                std::string name(PyUnicode_AsUTF8(types));
                parameters_types.push_back(name.c_str());
                // printf("C: list_param_types() = %s\n", name.c_str());
            }
	}
	else
	{
	    printf("ERROR: function list_param_types()\n");
	}
    }
    else
    {
        printf("ERROR: Module not imported\n");
    }
    return parameters_types;
}

void DyndspWrapper::getData(Context &m_context) {
    ip_address = &m_context.target_ip_address;
    //sys_name = m_context.system_name;
}

// Helper functions for validateIP() //
bool validateIP(std::string ip) {
    // split the string into tokens
    std::vector<std::string> list = split(ip, '.');
 
    // if the token size is not equal to four
    if (list.size() != 4) {
        return false;
    }
 
    // validate each token
    for (std::string str: list)
    {
        // verify that the string is a number or not, and the numbers
        // are in the valid range
        if (!isNumber(str) || stoi(str) > 255 || stoi(str) < 0) {
            return false;
        }
    }
    return true;
}

bool isNumber(const std::string &str) {
    return !str.empty() &&
        (str.find_first_not_of("[0123456789]") == std::string::npos);
}

// Function to split string `str` using a given delimiter
vector<string> split(const string &str, char delim)
{
    auto i = 0;
    vector<string> list;
 
    auto pos = str.find(delim);
 
    while (pos != string::npos)
    {
        list.push_back(str.substr(i, pos - i));
        i = ++pos;
        pos = str.find(delim, pos);
    }
 
    list.push_back(str.substr(i, str.length()));
 
    return list;
}