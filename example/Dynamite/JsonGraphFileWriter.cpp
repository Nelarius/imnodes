#include "JsonGraphFileWriter.h"

void JsonGraphFileWriter::writeToFile(Context& context) {
    // set up .json file
    /* method 1
    ofstream ofs("system.json");    // create output file
    OStreamWrapper osw(ofs);        // stream wrapper
    Writer<OStreamWrapper> writer(osw); // writter for the stream wrapper
    */
    // method 2
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
    block.SetObject();

    string name = "input";
    auto iter = std::find_if(
    context._blocks.begin(), context._blocks.end(), [name](Block& block) -> bool {
        return block.getName() == name;
    });
    block.AddMember("name", "input", allocator);

    Value input_chans(kArrayType);
    map<int, Port>::iterator it;
    for (it = iter->_inPorts.begin(); it != iter->_inPorts.end(); it++) {
        Value input_ch;
        input_ch.SetObject();
        //input_ch.AddMember("name", it->second.c_str(), allocator);
        input_ch.AddMember("name", "input", allocator);
        input_chans.PushBack(input_ch, allocator);
    }
    block.AddMember("input_channels", input_chans, allocator);

    Value output_chans(kArrayType);
    for (it = iter->_outPorts.begin(); it != iter->_outPorts.end(); it++) {
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
    /*
    iterate over blocks
    add each block to t
    block.SetObject();
    */
    jsonDoc.AddMember("dsp_blocks", dsp_blocks, allocator);
    // write to file
    jsonDoc.Accept(pwriter);
}