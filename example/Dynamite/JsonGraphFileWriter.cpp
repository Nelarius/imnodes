#include "JsonGraphFileWriter.h"

#define __STDC_WANT_LIB_EXT1__ 1
#define RAPIDJSON_HAS_STDSTRING 1

#include "block.h"
#include "graph.h"
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
#include <cstdlib>

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace rapidjson;
using namespace std;

void JsonGraphFileWriter::setSysName(Context& m_context) {
    sys_name = &m_context.system_name;
}

bool validate_sys_name(std::string sysname) {
    if (strcmp(sysname.c_str(), "") == 0) {
        return false;
    }
    return true;
}

void JsonGraphFileWriter::writeToFile(Context& context) {
    Graph graph = context.m_graph;
    setSysName(context); // get reference to system_name from m_context
    // set up .json file
    FILE* fp = fopen("system.json", "w");
    char buffer[65536];
    FileWriteStream fs(fp, buffer, sizeof(buffer));
    PrettyWriter<FileWriteStream> pwriter(fs);

    // create DOM document
    Document jsonDoc; // generate a DOM element document
    Document::AllocatorType &allocator = jsonDoc.GetAllocator();    // get allocator
    jsonDoc.SetObject();    // set the current document as an object, that is, the entire doc is an object type DOM element

    // populate data
    Value name;
    
    if (validate_sys_name(*sys_name)) {
        name = StringRef(*sys_name);
        jsonDoc.AddMember("name", name, allocator);
    } else {
        std::cerr << "ERROR : cannot serialize without system name" << std::endl;
    }

    if (graph._blocks.empty()) {
        std::cerr << "ERROR : no blocks in system" << std::endl;
    } else {
        // add global input, output, and scratch buffer channels
        Value input_channels(kArrayType);
        Value output_channels(kArrayType);
        Value scratch_buffers(kArrayType);
        Value channel;
        for (Block& b : graph._blocks) {
            if (b.getType() == "input") {
                for (auto &port : b._outPorts) {
                    channel.SetObject();
                    name = StringRef(port.second.name);
                    channel.AddMember("name", name, allocator);
                    input_channels.PushBack(channel, allocator);
                }
                jsonDoc.AddMember("input_channels", input_channels, allocator);
            }
            if (b.getType() == "output") {
                for (auto &port : b._inPorts) {
                    channel.SetObject();
                    name = StringRef(port.second.reference_name);
                    channel.AddMember("name", name, allocator);
                    output_channels.PushBack(channel, allocator);
                }
                jsonDoc.AddMember("output_channels", output_channels, allocator);
            }
        }

        for (Block& b : graph._blocks) {
            bool is_match = false;
            if (b.getType() == "input" || b.getType() == "output") {
                continue;
            }
            for (auto& p : b._outPorts) {
                for (Value::ConstValueIterator it = input_channels.Begin(); it != input_channels.End(); ++it) { 
                    printf("Input channel name : %s, Port name : %s\n", it->GetString(), p.second.name);
                    if (strcmp(it->MemberBegin()->value.GetString(), p.second.name) == 0) {
                        printf("is match\n"); 
                        is_match = true;
                        continue;
                    }
                }
                for (Value::ConstValueIterator it = output_channels.Begin(); it != output_channels.End(); ++it) {
                    printf("Output channel name : %s, Port name : %s\n", it->GetString(), p.second.name);
                    if (strcmp(it->MemberBegin()->value.GetString(), p.second.name) == 0) {
                        printf("is match\n"); 
                        is_match = true;
                        continue;
                    }
                }
                for (Value::ConstValueIterator it = scratch_buffers.Begin(); it != scratch_buffers.End(); ++it) {
                    printf("Scratch channel name : %s, Port name : %s\n", it->GetString(), p.second.name);
                    if (strcmp(it->MemberBegin()->value.GetString(), p.second.name) == 0) {
                        printf("is match\n"); 
                        is_match = true;
                        continue;
                    }
                }

                if (!is_match) {
                    channel.SetObject();
                    name = StringRef(p.second.name);
                    channel.AddMember("name", name, allocator);
                    scratch_buffers.PushBack(channel, allocator);
                }
            }
        }
        if (!scratch_buffers.Empty()) {
            jsonDoc.AddMember("scratch_buffers", scratch_buffers, allocator);
        }
        
 
        // add DSP blocks
        Value dsp_blocks(kArrayType);
        Value name;
        for (Block& b: graph._blocks) {
            // Skip over input and output blocks
            if ((0 == strcmp(b.getType().c_str(), "input")) || (0 == strcmp(b.getType().c_str(), "output"))) continue;
            
            Value block;
            block.SetObject();
            name = StringRef(b.name);
            block.AddMember("name", name, allocator);

            // add block input channels
            Value input_chans(kArrayType);
            map<int, Port>::iterator it;
            for (it = b._inPorts.begin(); it != b._inPorts.end(); it++) {
                Value input_ch;
                input_ch.SetObject();
                Value ref_name;
                ref_name = StringRef(it->second.reference_name);
                input_ch.AddMember("name", ref_name, allocator);
                input_chans.PushBack(input_ch, allocator);
            }
            block.AddMember("input_channels", input_chans, allocator);

            Value output_chans(kArrayType);
            for (it = b._outPorts.begin(); it != b._outPorts.end(); it++) {
                Value output_ch;
                output_ch.SetObject();
                name = StringRef(it->second.name);
                output_ch.AddMember("name", name, allocator);
                output_chans.PushBack(output_ch, allocator);
            }
            block.AddMember("output_channels", output_chans, allocator);

            // add block parameters
            Value param;
            param.SetObject();
            std::map<int, Parameter>::iterator itp;
            for (itp = b._parameters.begin(); itp != b._parameters.end(); itp++) {
                if (itp->second.name != "none") {
                    Value n; n = StringRef(itp->second.name.c_str());
                    Value v;
                    if (itp->second.type == "int") {
                        v.SetInt(std::strtol(itp->second.value, nullptr, 10));
                    } else if (itp->second.type == "float") {
                        v.SetFloat(*itp->second.value);
                    } else if (itp->second.type == "bool") {
                        std::istringstream is(itp->second.value);
                        bool b;
                        is >> std::boolalpha >> b;
                        v.SetBool(b);
                    } else {
                        v = StringRef(itp->second.value);
                    }
                    param.AddMember(n, v, allocator);
                }
            }
            block.AddMember(Value(b.getType().c_str(), b.getType().size(), allocator).Move(), param, allocator);
            dsp_blocks.PushBack(block, allocator);
        }
        jsonDoc.AddMember("dsp_blocks", dsp_blocks, allocator);
    }
    // write to file
    jsonDoc.Accept(pwriter);
    fclose(fp);
}