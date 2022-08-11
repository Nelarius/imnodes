#include "dyndsp_wrapper.h" 
#include <Python.h>
#include "pyhelper.h"
#include <vector>
#include <string>

using namespace std;

// Single initialization of Python interpreter
CPyInstance hInstance;

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
                string name(PyUnicode_AsUTF8(dsp_block));
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
                string name(PyUnicode_AsUTF8(control_block));
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
                string name(PyUnicode_AsUTF8(parameters));
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
                string name(PyUnicode_AsUTF8(types));
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