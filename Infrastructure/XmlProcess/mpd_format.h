#pragma once

struct represent
{
    int16_t id = 0;
    int bandwidth = 0;
    std::string media;
    std::string initialization;
};

struct adaptation_set
{
    std::string codecs;
    std::string mime_type;
    std::vector<represent> represents;
};

struct video_adaptation_set : adaptation_set
{
    int16_t x = 0;
    int16_t y = 0;
    int16_t width = 0;
    int16_t height = 0;
};

struct audio_adaptation_set : adaptation_set
{
    int sample_rate = 0;
};
