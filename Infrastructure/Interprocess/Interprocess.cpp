// Interprocess.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <string>
#include <fstream>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/containers/flat_map.hpp>


using namespace std::literals;
namespace interprocess = boost::interprocess;

const auto process_name = "Interprocess"s;

int main()
{



    return 0;
}

