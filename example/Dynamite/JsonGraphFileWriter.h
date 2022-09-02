#pragma once

#include <string>
#include "GraphFileWriter.h"
#include "context.h"

struct Port;

class JsonGraphFileWriter : public GraphFileWriter {
    std::string *sys_name;

    public:
        void setSysName(Context& m_context);

        // Serializes a graph to Pretty JSON
        // JSON saved to 'system.json' in project folder
        void writeToFile(Context& context);
};









