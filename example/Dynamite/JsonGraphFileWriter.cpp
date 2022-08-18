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

using namespace rapidjson;
using namespace std;

void JsonGraphFileWriter::writeToFile(Context& context) {
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
    Value s;
    jsonDoc.AddMember("name", "DSPSystemDynamite", allocator);

    Value in_ch_ar(kArrayType);
    for (Block& b : context._blocks) {
        if (b.getType() == "input") {
            map<int, Port>::iterator it;
            for (it = b._outPorts.begin(); it != b._outPorts.end(); it++) {
                Value channel;
                channel.SetObject();
                s = StringRef(it->second.name);
                channel.AddMember("name", s, allocator);
                in_ch_ar.PushBack(channel, allocator);
            }
            jsonDoc.AddMember("input_channels", in_ch_ar, allocator);
        }
    }

    Value out_ch_ar(kArrayType);
    for (Block& b : context._blocks) {
        if (b.getType() == "output") {
            map<int, Port>::iterator it;
            for (it = b._inPorts.begin(); it != b._inPorts.end(); it++) {
                Value channel;
                channel.SetObject();
                Value o;
                o = StringRef(it->second.reference_name);
                channel.AddMember("name", o, allocator);
                out_ch_ar.PushBack(channel, allocator);
            }
            jsonDoc.AddMember("output_channels", out_ch_ar, allocator);
        }
    }

    Value dsp_blocks(kArrayType);
    Value name;
    for (Block& b: context._blocks) {
        // Skip over input and output blocks
        if ((0 == strcmp(b.getType().c_str(), "input")) || (0 == strcmp(b.getType().c_str(), "output"))) continue;
        
        Value block;
        block.SetObject();
        name = StringRef(b.name);
        block.AddMember("name", name, allocator);

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

        Value param;
        param.SetObject();
        map<int,Parameter>::iterator itp;
        for (itp = b._parameters.begin(); itp != b._parameters.end(); itp++) {
            if (itp->second.name != "none") {
                Value n; n = StringRef(itp->second.name.c_str());
                Value v; v = StringRef(itp->second.value);
                param.AddMember(n, v, allocator);
            }
        }
    
        block.AddMember(Value(b.getType().c_str(), b.getType().size(), allocator).Move(), param, allocator);
        dsp_blocks.PushBack(block, allocator);
    }
    jsonDoc.AddMember("dsp_blocks", dsp_blocks, allocator);

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