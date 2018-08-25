#pragma once
#include "mpd_format.h"

class mpd_parser
{
    std::vector<video_adaptation_set> video_adaptation_sets_;
    std::unique_ptr<audio_adaptation_set> audio_adaptation_set_;

public:
    static const inline std::regex duration_pattern{ R"(PT((\d+)H)?((\d+)M)?(\d+\.\d+)S)" };
    static std::chrono::milliseconds parse_duration(std::string_view duration);

    std::string title;
    std::chrono::milliseconds min_buffer_time;
    std::chrono::milliseconds max_segment_duration;
    std::chrono::milliseconds presentation_time;
    int16_t width = 0;
    int16_t height = 0;

    explicit mpd_parser(std::string_view xml_text);

    video_adaptation_set& video_set(int column, int row) const;
    audio_adaptation_set& audio_set() const;
};

