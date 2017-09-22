// FrameScale.cpp : Defines the entry point for the console application.


#include "stdafx.h"
#define __STDC_CONSTANT_MACROS
#include "scaler.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <chrono>


int main()
{
    std::ifstream fin{ "D:/test.nv12",std::ios::binary };
    std::ofstream fout{ "E:/testscale.rgb",std::ios::binary };
    assert(fin&&fout);
    scaler converter;
    converter.push<scaler::port>(3840, 1920, pixel_format::nv12);       //source      
    converter.push<scaler::port>(3840, 1920, pixel_format::rgb24);      //destination
    const auto src_framesize = converter.front().size();
    const auto dest_framesize = converter.back().size();

    struct time_guard
    {
        std::chrono::time_point<std::chrono::steady_clock> time_mark;
        time_guard() { time_mark = std::chrono::steady_clock::now(); }
        ~time_guard()
        {
            std::cout << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - time_mark).count() << "secs\n";
        }
    }guard;

    auto data = std::make_unique<char[]>(src_framesize);
    while(fin.read(data.get(),src_framesize))
    {
        auto scaled_data = converter.run_one(move(data));
        //fout.write(scaled_data.get(), dest_framesize);
    }

    fin.close();
    fout.close();

}

