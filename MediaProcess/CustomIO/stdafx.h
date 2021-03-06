// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define NOMINMAX
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#define _SCL_SECURE_NO_WARNINGS
#define __STDC_CONSTANT_MACROS
#define _SCL_SECURE_NO_WARNINGS

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

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <type_traits>
#include <optional>
#include <variant>
#include <atomic>
#include <thread>
#include <mutex>
#include <future>
#include <string_view>
#include <filesystem>
#include <fstream>

using namespace std::literals;

namespace std::filesystem
{
    using namespace std::experimental::filesystem;
}

namespace core
{
    template<typename T>
    std::string type_shortname(std::add_pointer_t<T> = nullptr)
    {
        std::string type_name{ typeid(T).name() };
        type_name.erase(0, type_name.find_last_of(": ") + 1);
        return type_name;
    }
    template <typename T>
    std::reference_wrapper<T> make_null_reference_wrapper()
    {
        static void* lval_nullptr = nullptr;
        return std::reference_wrapper<T>{ *reinterpret_cast<std::add_pointer_t<T>&>(lval_nullptr) };
    }
}

#include "exception.h"
#include "verify.hpp"
#include "ffmpeg.h"
#include "context.h"

#pragma comment(lib, "avformat")
#pragma comment(lib, "avcodec")
#pragma comment(lib, "avutil")
#pragma comment(lib, "swscale")