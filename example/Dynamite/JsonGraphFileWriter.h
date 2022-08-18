#pragma once

#include "GraphFileWriter.h"

struct Port;

class JsonGraphFileWriter : public GraphFileWriter {
    public:

        void writeToFile(Context& context);
};









