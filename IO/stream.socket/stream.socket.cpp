// stream.socket.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <laser/basis.h>
#include "method/seqence.h"

const auto fileName = "Harbor_8192x4096_30fps_8bit_420_erp.yuv";
const vector<file::path> fileDir{ 
	file::path{ "c:/" } / fileName,
	file::path{ "d:/" } / fileName 
};


int main()
{
	assert(file::exists(fileDir[0]) && file::exists(fileDir[1]));
	file::ofstream fout{ "D:/DrivingInCountry_test_903frames.yuv",ios::binary | ios::trunc };
	file::ifstream fin{ "D:/DrivingInCountry_3840x1920_30fps_8bit_420_erp.yuv",ios::binary };
	assert(fin);
	assert(fout);
	const auto frameSz = 3840 * 1920 / 2 * 3;
	scoped_ptr<char> ptr{ new char[frameSz] };
	while(fin.read(ptr.get(),frameSz))
	{
		fout.write(ptr.get(), frameSz);
		fout.write(ptr.get(), frameSz);
		fout.write(ptr.get(), frameSz);
	}
	return 0;
}

