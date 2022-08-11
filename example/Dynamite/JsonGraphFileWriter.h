#pragma once

#define __STDC_WANT_LIB_EXT1__ 1
#include "GraphFileWriter.h"
#include "block.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace rapidjson;
using namespace std;

struct Port;

class JsonGraphFileWriter : public GraphFileWriter {
    public:
        //~JsonGraphFileWriter() { cout << "Deconstructor for json file writer called." << endl; }
        void writeToFile(Context& context);
};









