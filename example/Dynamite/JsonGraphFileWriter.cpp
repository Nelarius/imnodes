#include "JsonGraphFileWriter.h"

/*
ERRORS: 
- finding it difficult to change block input/output names and adding new ones -> clicking on the mpp boxes is weird
- must use string literal with AddmMember(), can't assign specific block values to value arg
*/

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
    jsonDoc.AddMember("name", "DSPSystemDynamite", allocator);
    Value in_ch_ar(kArrayType);
    Value channel;
    channel.SetObject();
    channel.AddMember("name", "IN_L", allocator);
    in_ch_ar.PushBack(channel, allocator);
    channel.SetObject();
    channel.AddMember("name", "IN_R", allocator);
    in_ch_ar.PushBack(channel, allocator);
    jsonDoc.AddMember("input_channels", in_ch_ar, allocator);

    Value dsp_blocks(kArrayType); // AddMember("dsp_blocks", dsp_blocks, jsonDoc.GetAllocator()) is the last line
    Value block;

    for (Block& b: context._blocks) {
        block.SetObject();
        block.AddMember("name", "block name", allocator);

        Value input_chans(kArrayType);
        map<int, Port>::iterator it;
        for (it = b._inPorts.begin(); it != b._inPorts.end(); it++) {
            Value input_ch;
            input_ch.SetObject();
            /*
            string s = it->second.type;
            Value type(s.c_str(), s.size(), allocator);
            input_ch.AddMember("name", s, allocator); */
            input_ch.AddMember("name", "input", allocator);
            input_chans.PushBack(input_ch, allocator);
        }
        block.AddMember("input_channels", input_chans, allocator);

        Value output_chans(kArrayType);
        for (it = b._outPorts.begin(); it != b._outPorts.end(); it++) {
            Value output_ch;
            output_ch.SetObject();
            //output_ch.AddMember("name", it->second.c_str(), allocator);
            output_ch.AddMember("name", "output", allocator);
            output_chans.PushBack(output_ch, allocator);
        }
        block.AddMember("output_channels", output_chans, allocator);

        /*
        Value param;
        param.SetObject();
        map<int,Parameter>::iterator itp;
        for (itp = iter->_parameters.begin(); itp != iter->_parameters.end(); itp++) {
            param.AddMember(iter->second.name, iter->second.value, allocator);
        }
        block.AddMember(iter->getName, param, allocator);
        */

       dsp_blocks.PushBack(block, allocator);
    }
    jsonDoc.AddMember("dsp_blocks", dsp_blocks, allocator);

    // write to file
    jsonDoc.Accept(pwriter);
}