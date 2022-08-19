#include "JsonGraphFileWriter.h"

#define __STDC_WANT_LIB_EXT1__ 1
#define RAPIDJSON_HAS_STDSTRING 1

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
#include <cstdlib>

using namespace rapidjson;
using namespace std;

void JsonGraphFileWriter::getSysName(Context& m_context) {
    sys_name = &m_context.system_name;
}

bool validate_sys_name(std::string sysname) {
    if (strcmp(sysname.c_str(), "") == 0) {
        return false;
    }
    return true;
}

void JsonGraphFileWriter::writeToFile(Context& context) {
    getSysName(context);
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
    //*/
    if (context._blocks.empty()) {
        std::cerr << "ERROR : no blocks in system" << std::endl;
    } else {
        // add global input, output, and scratch buffer channels
        Value input_channels(kArrayType);
        Value output_channels(kArrayType);
        Value scratch_buffers(kArrayType);
        Value channel;
        for (Block& b : context._blocks) {
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
        /*
        for (Block& b : context._blocks) {
            for (auto &port : b._outPorts) {
                auto member_val = input_channels.FindMember(port.second.name); 
                auto member_val_out = output_channels.FindMember(port.second.name);

                if ((strcmp(port.second.name, member_val) || (port.second.name != member_val_out)) {
                    channel.SetObject();
                    name = StringRef(port.second.name);
                    channel.AddMember("name", name, allocator);
                    scratch_buffers.PushBack(channel, allocator);
                }
            }
        }
        jsonDoc.AddMember("scratch_buffers", scratch_buffers, allocator);
        //*/
        // add DSP blocks
        Value dsp_blocks(kArrayType);
        Value name;
        for (Block& b: context._blocks) {
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

            // add block output channels
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
            map<int,Parameter>::iterator itp;
            for (itp = b._parameters.begin(); itp != b._parameters.end(); itp++) {
                if (itp->second.name != "none") {
                    Value n; n = StringRef(itp->second.name.c_str());
                    Value v;
                    if (itp->second.type == "int") {
                        v.SetInt(std::strtol(itp->second.value, nullptr, 10));
                    } else if (itp->second.type == "float") {
                        v.SetFloat(*itp->second.value);
                    } else if (itp->second.type == "bool") {
                        v.SetBool(*itp->second.value);
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

// Helper functions

void writeInputChannels(Context& context, Document& jsonDoc, Document::AllocatorType& allocator) {
    Value input_channels(kArrayType);
    for (Block& b : context._blocks) {
        if (b.getType() == "input") {
            std::map<int, Port>::iterator it;
            for (it = b._outPorts.begin(); it != b._outPorts.end(); it++) {
                Value channel;
                channel.SetObject();
                Value name;
                name = StringRef(it->second.name);
                channel.AddMember("name", name, allocator);
                input_channels.PushBack(channel, allocator);
            }
            jsonDoc.AddMember("input_channels", input_channels, allocator);
        }
    }
}

void writeOutputChannels(Context& context, Document& jsonDoc, Document::AllocatorType& allocator) {
    Value output_channels(kArrayType);
    for (Block& b : context._blocks) {
        if (b.getType() == "output") {
            map<int, Port>::iterator it;
            for (it = b._inPorts.begin(); it != b._inPorts.end(); it++) {
                Value channel;
                channel.SetObject();
                Value name;
                name = StringRef(it->second.reference_name);
                channel.AddMember("name", name, allocator);
                output_channels.PushBack(channel, allocator);
            }
            jsonDoc.AddMember("output_channels", output_channels, allocator);
        }
    }
}