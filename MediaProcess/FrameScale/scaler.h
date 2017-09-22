#pragma once
#include <memory>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <vector>
#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
}

struct buffer_view
{
    char* ptr;
    size_t size;
};

enum class pixel_format
{   //cover common pixel usage
    nv12 = AV_PIX_FMT_NV12,
    nv16 = AV_PIX_FMT_NV16,
    rgb24 = AV_PIX_FMT_RGB24,
    rgba = AV_PIX_FMT_RGBA,
    yuv420 = AV_PIX_FMT_YUV420P,
    yuv422 = AV_PIX_FMT_YUV422P,
    uyvy= AV_PIX_FMT_UYVY422
};

class scaler
{    
private:
    //base class, currently regardless of filter, which won't inherit all pipe data members
    //TODO:filter will be defined in the future, required by pixel reprojection
    struct pipe
    {
        int const width, height;
        pixel_format const pix_fmt;
        uint8_t *data[4];
        int linesize[4];
        int frame_size;
        pipe(int const w, int const h, pixel_format const fmt);
        int size() const noexcept { return frame_size; }
        virtual ~pipe() = default;
    };
    //private data member
    SwsContext *sws_ctx_;
    std::vector<std::unique_ptr<pipe>> pipeline_;
public:
    struct port :pipe
    {
        template<typename... Ts>
        explicit port(Ts&&... args) noexcept(false);
        ~port() { av_freep(&data[0]); }
    };

    scaler();
    //@param width,height,pixel_format
    //push order: port->filter->filter->...->filter->port
    //after output port is pushed, swscale context would be set automatically
    template<typename Port, typename... Ts>
    std::enable_if_t<std::is_base_of_v<pipe, Port>>
        push(Ts&&... args);
    scaler::pipe& operator[](size_t const pos) const;
    scaler::pipe& front() const;
    scaler::pipe& back() const;
    std::unique_ptr<char[]> run_one(const std::unique_ptr<char[]>& data);
    ~scaler() { sws_freeContext(sws_ctx_); }
};


template <typename ... Ts>
scaler::port::port(Ts&&... args) noexcept(false)
    : pipe(std::forward<Ts>(args)...)
{
    frame_size = av_image_alloc(data, linesize, width, height,
        static_cast<AVPixelFormat>(pix_fmt), 16);
 
    if (frame_size < 0) throw std::runtime_error(
        std::string{ "raw image allocation fail, pixel format: " }+
        av_get_pix_fmt_name(static_cast<AVPixelFormat>(pix_fmt)));
}

template <typename Pipe, typename... Ts>
std::enable_if_t<std::is_base_of_v<scaler::pipe, Pipe>> 
scaler::push(Ts&&... args)
{
    auto pipe = std::make_unique<port>(std::forward<Ts>(args)...);
    pipeline_.push_back(move(pipe));
    if (std::is_same_v<Pipe, port>&&pipeline_.size() > 1)
    {
        auto& src = *pipeline_.front();
        auto& dest = *pipeline_.back();
        sws_ctx_ = sws_getContext(
            src.width, src.height, static_cast<AVPixelFormat>(src.pix_fmt),
            dest.width, dest.height, static_cast<AVPixelFormat>(dest.pix_fmt),
            SWS_X, nullptr, nullptr, nullptr);
        if (!sws_ctx_) throw std::runtime_error("create scaling context");
    }
}

//SWS_BILINEAR 22
//SWS_FAST_BILINEAR 21
//SWS_AREA 22
//SWS_BICUBIC 37
//SWS_BICUBLIN 23
//SWS_DIRECT_BGR 37
//SWS_GAUSS 37
//SWS_POINT 22
//SWS_X 23