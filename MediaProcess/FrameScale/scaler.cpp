#include "stdafx.h"
#include "scaler.h"


scaler::pipe::pipe(int const w, int const h, pixel_format const fmt)
    : width{ w }, height{ h }, pix_fmt{ fmt }, frame_size{ 0 } {}



scaler::scaler() : sws_ctx_{ nullptr }
{
    std::cout << "ctor" << sizeof(pipe) << std::endl;
    std::cout << "ctor" << sizeof(port) << std::endl;
    pipeline_.reserve(2);
}

scaler::pipe& scaler::operator[](size_t const pos) const
{
    return *pipeline_[pos];
}

scaler::pipe& scaler::front() const
{
    return *pipeline_.front();
}

scaler::pipe& scaler::back() const
{
    return *pipeline_.back();

}

std::unique_ptr<char[]> scaler::run_one(const std::unique_ptr<char[]>& data)
{

    auto& src = *pipeline_.front();
    auto& dest = *pipeline_.back();
    auto result_ptr = std::make_unique<char[]>(dest.size());
    std::copy(data.get(), data.get() + src.size(), src.data[0]);    //not effective approach, just use for ease
    sws_scale(sws_ctx_, (const uint8_t * const*)src.data,
        src.linesize, 0, src.height, dest.data, dest.linesize);
    std::copy(dest.data[0], dest.data[0] + dest.size(), result_ptr.get());

    return result_ptr;
}

