#pragma once

#include <string>
#include "GraphFileWriter.h"
#include "context.h"

struct Port;

class JsonGraphFileWriter : public GraphFileWriter {
    std::string *sys_name;

    public:
        void getSysName(Context& m_context);
        void writeToFile(Context& context);
};









