#include "dynamite.h"
#include <Python.h>
#include "pyhelper.h"
#include <stdio.h>

Dynamite::Dynamite() { } 

void Dynamite::init() {
    m_ui.init();
    m_context.init();
}

int Dynamite::python_test() {
	CPyInstance hInstance;

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\"./example/Dynamite\")");
    PyRun_SimpleString("print(sys.path)");

	CPyObject pName = PyUnicode_FromString("run_test");
	CPyObject pModule = PyImport_Import(pName);

	if(pModule)
	{
		CPyObject pFunc = PyObject_GetAttrString(pModule, "result");
		if(pFunc && PyCallable_Check(pFunc))
		{
			CPyObject pValue = PyObject_CallObject(pFunc, NULL);

			printf("C: result() = %ld\n", PyLong_AsLong(pValue));
		}
		else
		{
			printf("ERROR: function getInteger()\n");
		}

	}
	else
	{
		printf("ERROR: Module not imported\n");
	}

	return 0;
}

bool Dynamite::show(bool done) {
    return done = m_ui.show(done, m_context); 
}

void Dynamite::exit() {
    m_ui.exit();
}



