#pragma once
#include "mpd_format.h"

class mpd_parser
{
    std::vector<video_represent> video_represents_;
    std::unique_ptr<audio_represent> audio_represent_;

public:
    static const inline std::regex  duration_pattern{ R"(PT((\d+)H)?((\d+)M)?(\d+\.\d+)S)" };
    static std::chrono::milliseconds parse_duration(const char * duration) {
        std::cmatch matches;
        if (std::regex_match(duration, matches, duration_pattern)) {
            const auto hour = matches[2].matched ? std::stoi(matches[2].str()) : 0;
            const auto minute = matches[4].matched ? std::stoi(matches[4].str()) : 0;
            const auto second = matches[5].matched ? std::stod(matches[5].str()) : 0;
            return 1ms*boost::numeric_cast<int64_t>(1000 * (hour * 3600 + minute * 60 + second));
        }
        throw std::runtime_error{ __FUNCTION__ "$unmatch" };
    }

    std::string title;
    std::chrono::milliseconds min_buffer_time;
    std::chrono::milliseconds presentation_time;

    mpd_parser();
    ~mpd_parser();
};

