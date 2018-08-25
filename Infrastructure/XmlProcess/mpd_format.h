#pragma once

struct mpd
{
    std::string title;
    std::chrono::duration<int64_t> duration;
private:
    std::chrono::duration<int64_t> parse_total_duration();
    std::string parse_concrete_media();
};

struct represent_base
{
    int16_t id = 0;
    int bandwidth = 0;
    std::string codecs;
    std::string mime_type;
    std::string initialization;
};

struct video_represent : represent_base
{
    int8_t row = 0;
    int8_t column = 0;
    int16_t width = 0;
    int16_t height = 0;
};

struct audio_represent : represent_base
{
    int sample_rate = 0;
};


class mpd_format
{
public:
    mpd_format();
    ~mpd_format();
};

