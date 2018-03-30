#include "stdafx.h"
#include "context.h"

av::io_context::io_context(func_tuple&& io_functions, const uint32_t buf_size, const bool buf_writable)
    : io_interface_(make_io_interface(std::move(io_functions)))
    , handle_(avio_alloc_context(static_cast<uint8_t*>(av_malloc(buf_size)),
        buf_size, buf_writable, io_interface_.get(),
        io_interface_->readable() ? read_func_delegate : nullptr,
        io_interface_->writable() ? write_func_delegate : nullptr,
        io_interface_->seekable() ? seek_func_delegate : nullptr),
        [](pointer ptr) { av_freep(&ptr->buffer);  av_freep(&ptr); })
{
    std::cerr << "constructor\n";
}

av::io_context::pointer av::io_context::operator->() const
{
    return handle_.get();
}

av::io_context::operator bool() const
{
    return handle_ != nullptr && io_interface_ != nullptr;
}

std::shared_ptr<av::io_context::io_interface>
av::io_context::make_io_interface(func_tuple&& io_functions)
{
    struct io_interface_impl : io_interface
    {
        explicit io_interface_impl(
            std::function<int(uint8_t*, int)>&& rfunc = nullptr,
            std::function<int(uint8_t*, int)>&& wfunc = nullptr,
            std::function<int64_t(int64_t, int)>&& sfunc = nullptr)
            : read_func(std::move(rfunc))
            , write_func(std::move(wfunc))
            , seek_func(std::move(sfunc))
        {
        }
        int read(uint8_t* buffer, const int size) override final
        {
            return readable() ? read_func(buffer, size) : std::numeric_limits<int>::min();
        }
        int write(uint8_t* buffer, const int size) override final
        {
            return writable() ? write_func(buffer, size) : std::numeric_limits<int>::min();
        }
        int64_t seek(const int64_t offset, const int whence) override final
        {
            return seekable() ? seek_func(offset, whence) : std::numeric_limits<int64_t>::min();
        }
        bool readable() override final { return read_func != nullptr; }
        bool writable() override final { return write_func != nullptr; }
        bool seekable() override final { return seek_func != nullptr; }
        std::function<int(uint8_t*, int)> read_func;
        std::function<int(uint8_t*, int)> write_func;
        std::function<int64_t(int64_t, int)> seek_func;
    };
    return std::make_shared<io_interface_impl>(
        std::move(std::get<0>(io_functions)),
        std::move(std::get<1>(io_functions)),
        std::move(std::get<2>(io_functions)));
}

int av::io_context::read_func_delegate(void* opaque, uint8_t* buffer, int size)
{
    return static_cast<io_interface*>(opaque)->read(buffer, size);
}

int av::io_context::write_func_delegate(void* opaque, uint8_t* buffer, int size)
{
    return static_cast<io_interface*>(opaque)->write(buffer, size);
}

int64_t av::io_context::seek_func_delegate(void* opaque, int64_t offset, int whence)
{
    return static_cast<io_interface*>(opaque)->seek(offset, whence);
}

av::format_context::format_context(std::variant<source, sink> io)
    : handle_(nullptr)
    , io_handle_()
{
    std::visit([this](auto&& arg) constexpr
    {
        register_all();
        if constexpr (std::is_same_v<source, std::decay_t<decltype(arg)>>)
        {
            pointer ptr = nullptr;
            core::verify(avformat_open_input(&ptr, arg.url.c_str(), nullptr, nullptr));
            handle_.reset(ptr, [](pointer p) { avformat_close_input(&p); });
            core::verify(avformat_find_stream_info(ptr, nullptr));   // 60ms+
#ifdef _DEBUG
            av_dump_format(ptr, 0, ptr->filename, 0);
#endif
        }
        else
        {
            // TODO: SINK BRANCH
            throw core::not_implemented_error{};
        }
    }, io);
}

av::format_context::format_context(io_context io, source::format iformat)
    : handle_(nullptr)
    , io_handle_(std::move(io))
{
    register_all();
    auto ptr = avformat_alloc_context();
    ptr->pb = get_pointer(io_handle_);
    core::verify(avformat_open_input(&ptr, nullptr, av_find_input_format(iformat.data()), nullptr));
    handle_.reset(ptr, [](pointer p) { avformat_close_input(&p); });
    core::verify(avformat_find_stream_info(ptr, nullptr));   
#ifdef _DEBUG
    av_dump_format(ptr, 0, ptr->filename, 0);
#endif
}

av::format_context::pointer av::format_context::operator->() const
{
    return handle_.get();
}

av::format_context::operator bool() const
{
    return handle_ != nullptr;
}

av::stream av::format_context::demux(const media::type media_type) const
{
    return stream{ handle_->streams[av_find_best_stream(handle_.get(), media_type, -1, -1, nullptr, 0)] };
}

std::pair<av::codec, av::stream> av::format_context::demux_with_codec(const media::type media_type) const
{
    codec::pointer cdc = nullptr;
    const auto ptr = handle_.get();
    const auto index = av_find_best_stream(ptr, media_type, -1, -1, &cdc, 0);
    return std::make_pair(codec{ cdc }, stream{ ptr->streams[index] });
}

av::packet av::format_context::read(const std::optional<media::type> media_type) const
{
    packet pkt;
    while (av_read_frame(handle_.get(), get_pointer(pkt)) == 0
        && media_type.has_value()
        && handle_->streams[pkt->stream_index]->codecpar->codec_type != media_type)
    {
        pkt.unref();
    }
    return pkt;
}

std::vector<av::packet> av::format_context::read(const size_t count, std::optional<media::type> media_type) const
{
    std::vector<packet> packets; packets.reserve(count);
    //std::generate_n(std::back_inserter(packets), count, std::bind(&format_context::read, this, media_type));
    std::generate_n(std::back_inserter(packets), count, [this, media_type] { return read(media_type); });
    return packets;
}

av::codec_context::codec_context(codec codec, const stream stream, const unsigned threads)
    : handle_(avcodec_alloc_context3(get_pointer(codec)), [](pointer p) { avcodec_free_context(&p); })
    , stream_(stream)
    , state_()
{
    core::verify(avcodec_parameters_to_context(handle_.get(), stream_->codecpar));
    core::verify(av_opt_set_int(handle_.get(), "refcounted_frames", 1, 0));
    core::verify(av_opt_set_int(handle_.get(), "threads", threads, 0));
    core::verify(avcodec_open2(handle_.get(), get_pointer(codec), nullptr));
}

av::codec_context::pointer av::codec_context::operator->() const
{
    return handle_.get();
}

av::codec_context::operator bool() const
{
    return handle_ != nullptr;
}

bool av::codec_context::valid() const
{
    return !state_.flushed;
}

int64_t av::codec_context::decoded_count() const
{
    return state_.count;
}

int64_t av::codec_context::frame_count() const
{
    return stream_->nb_frames;
}

std::vector<av::frame> av::codec_context::decode(const packet& packets) const
{
    if (std::exchange(state_.flushed, packets.empty()))
        throw std::logic_error{ "prohibit multiple codec context flush" };
    if (stream_.index() != packets->stream_index)
        throw std::invalid_argument{ "prohibt decode disparate stream" };
    std::vector<frame> decoded_frames;
    decoded_frames.reserve(packets.empty() ? 10 : 1);
    core::verify(avcodec_send_packet(get_pointer(handle_), get_pointer(packets)));
    frame current;
    while (avcodec_receive_frame(get_pointer(handle_), get_pointer(current)) == 0)
        decoded_frames.push_back(std::exchange(current, frame{}));
    state_.count += decoded_frames.size();
    return decoded_frames;
}