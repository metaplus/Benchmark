// CustomIO.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main(int argc, char *argv[])
{
    static std::ifstream ifs{ "C:/Media/H264/MercedesBenz.h264" ,std::ios::in | std::ios::binary };
    core::verify(ifs.good());
    av::io_context io{ { [](uint8_t* buffer, int size)->int
    {
        //std::cerr << "start successor reading\n";
        ifs.read(reinterpret_cast<char*>(buffer), size);
        //std::cerr << "ifs read " << ifs.gcount() << "/" << size << "\n";
        return ifs.gcount();
    },nullptr,nullptr } };

    av::format_context format{ io,"h264"sv };
    auto[cdc, srm] = format.demux_with_codec(av::media::video{});
    av::codec_context codec{ cdc,srm };
    av::packet packet;
    uint64_t count = 0;
    while (!(packet = format.read(std::nullopt)).empty())
    {
        auto frame = codec.decode(packet);
        std::cout << "decoded count" << ++count << "\n";
    }
    return 0;
}