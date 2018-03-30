// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once
#include "targetver.h"
#include <stdio.h>
#include <tchar.h>
// TODO: reference additional headers your program requires here
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <cassert>
#include <cstdlib>
#include <cstring>
extern "C" {
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/hwcontext.h>
#include <libavutil/file.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/dxva2.h>
#include <libswscale/swscale.h>
#include <libavformat/avio.h>
#include <libavformat/avformat.h>

}
#define BOOST_THREAD_VERSION 4
#define BOOST_FILESYSTEM_NO_DEPRECATED 
#define BOOST_USE_WINDOWS_H    
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

#pragma comment(lib, "avformat")
#pragma comment(lib, "avcodec")
#pragma comment(lib, "avutil")
#pragma comment(lib, "swscale")