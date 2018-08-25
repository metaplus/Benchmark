#include "stdafx.h"
#include "mpd_parser.h"
#include <tinyxml2.h>
#include <cassert>
#include <algorithm>
#include <execution>
#include <array>
#include <folly/String.h>
#include <boost/multi_array.hpp>
#define BOOST_THREAD_VERSION 4
#include <boost/thread.hpp>

struct result
{
    template<typename U>
    result& operator=(U&& result) {
        using native_type = typename std::decay<U>::type;
        if constexpr (std::is_pointer<native_type>::value) {
            assert(result != nullptr);
        } else if constexpr (std::is_same<bool, native_type>::value) {
            assert(result);
        } else {
            throw std::runtime_error{ "unreachable branch" };
        }
        return *this;
    }
};

struct xml_result : result
{
    template<typename U>
    xml_result& operator=(U&& result) {
        using native_type = typename std::decay<U>::type;
        if constexpr (std::is_same<tinyxml2::XMLError, native_type>::value) {
            assert(result == tinyxml2::XMLError::XML_SUCCESS);
        } else {
            return static_cast<struct result*>(this)->operator=(result);
        }
        return *this;
    }
};

static thread_local xml_result result;

std::chrono::milliseconds mpd_parser::parse_duration(std::string_view duration) {
    std::cmatch matches;
    if (std::regex_match(duration.data(), matches, duration_pattern)) {
        using namespace std::chrono;
        const hours t1{ matches[2].matched ? std::stoi(matches[2].str()) : 0 };
        const minutes t2{ matches[4].matched ? std::stoi(matches[4].str()) : 0 };
        const milliseconds t3{ boost::numeric_cast<int64_t>(
           matches[5].matched ? 1000 * std::stod(matches[5].str()) : 0) };
        return t1 + t2 + t3;
    }
    throw std::runtime_error{ __FUNCTION__ "$unmatch" };
}

std::vector<tinyxml2::XMLElement*> list_next_sibling(const char* name,
                                                     tinyxml2::XMLElement* first_element) {
    std::vector<tinyxml2::XMLElement*> sibling{ first_element };
    auto* next = first_element->NextSiblingElement();
    while (next != nullptr) {
        sibling.push_back(next);
        next = next->NextSiblingElement(name);
    }
    return sibling;
}

std::array<int, 6> split_spatial_description(std::string_view srd) {
    std::array<int, 6> spatial{};
    srd.remove_prefix(srd.find(',') + 1);
    folly::splitTo<int>(',', srd, spatial.begin(), false);
    return spatial;
}

void initialize(std::vector<video_adaptation_set>& video_adaptation_sets,
                std::vector<tinyxml2::XMLElement*>::iterator element_begin,
                std::vector<tinyxml2::XMLElement*>::iterator element_end) {
    video_adaptation_sets.resize(std::distance(element_begin, element_end));
    std::transform(std::execution::par, element_begin, element_end, video_adaptation_sets.begin(),
                   [](tinyxml2::XMLElement* element) {
                       video_adaptation_set adaptation_set;
                       auto represents = list_next_sibling("Representation", element->FirstChildElement("Representation"));
                       adaptation_set.codecs = represents.front()->Attribute("codecs");
                       adaptation_set.mime_type = represents.front()->Attribute("mimeType");
                       adaptation_set.width = boost::numeric_cast<int16_t>(std::stoi(element->Attribute("maxWidth")));
                       adaptation_set.height = boost::numeric_cast<int16_t>(std::stoi(element->Attribute("maxHeight")));
                       auto[x, y, w, h, total_w, total_h] = split_spatial_description(
                           element->FirstChildElement("SupplementalProperty")->Attribute("value"));
                       adaptation_set.x = boost::numeric_cast<int16_t>(x);
                       adaptation_set.y = boost::numeric_cast<int16_t>(y);
                       adaptation_set.represents.resize(represents.size());
                       std::transform(std::execution::par, represents.begin(), represents.end(), adaptation_set.represents.begin(),
                                      [](tinyxml2::XMLElement* element) {
                                          represent represent;
                                          represent.id = boost::numeric_cast<int16_t>(std::stoi(element->Attribute("id")));
                                          represent.bandwidth = boost::numeric_cast<int>(std::stoi(element->Attribute("bandwidth")));
                                          represent.media = element->FirstChildElement("SegmentTemplate")->Attribute("media");
                                          represent.initialization = element->FirstChildElement("SegmentTemplate")->Attribute("initialization");
                                          return represent;
                                      });
                       return adaptation_set;
                   });
}

void initialize(std::unique_ptr<audio_adaptation_set>& audio_adaptation_set, tinyxml2::XMLElement* element) {
    element = element->FirstChildElement("Representation");
    audio_adaptation_set = std::make_unique<struct audio_adaptation_set>();
    audio_adaptation_set->codecs = element->Attribute("codecs");
    audio_adaptation_set->mime_type = element->Attribute("mimeType");
    represent represent;
    represent.id = boost::numeric_cast<int16_t>(std::stoi(element->Attribute("id")));
    represent.bandwidth = boost::numeric_cast<int>(std::stoi(element->Attribute("bandwidth")));
    represent.media = element->FirstChildElement("SegmentTemplate")->Attribute("media");
    represent.initialization = element->FirstChildElement("SegmentTemplate")->Attribute("initialization");
    audio_adaptation_set->represents.push_back(std::move(represent));
    audio_adaptation_set->sample_rate = std::stoi(element->Attribute("audioSamplingRate"));
}

void initialize(int16_t& width, int16_t& height, tinyxml2::XMLElement* element) {
    auto[x, y, w, h, total_w, total_h] = split_spatial_description(
        element->FirstChildElement("SupplementalProperty")->Attribute("value"));
    width = total_w;
    height = total_h;
}

mpd_parser::mpd_parser(std::string_view xml_text) {
    tinyxml2::XMLDocument document;
    result = document.Parse(xml_text.data());
    auto* xml_root = document.RootElement();
    presentation_time = parse_duration(xml_root->Attribute("mediaPresentationDuration"));
    min_buffer_time = parse_duration(xml_root->Attribute("minBufferTime"));
    max_segment_duration = parse_duration(xml_root->Attribute("maxSegmentDuration"));
    title = xml_root->FirstChildElement("ProgramInformation")->FirstChildElement("Title")->GetText();
    auto adaptation_sets = list_next_sibling("AdaptationSet",
                                             xml_root->FirstChildElement("Period")->FirstChildElement("AdaptationSet"));
    auto audio_set_iter = std::remove_if(adaptation_sets.begin(), adaptation_sets.end(),
                                         [](tinyxml2::XMLElement* element) {
                                             std::string_view mime = element->FirstChildElement("Representation")->Attribute("mimeType");
                                             return mime.find("audio") != std::string_view::npos;
                                         });
    auto futures = {
        boost::async([&] { initialize(video_adaptation_sets_, adaptation_sets.begin(), audio_set_iter); }),
        boost::async([&] { initialize(audio_adaptation_set_, *audio_set_iter); }),
        boost::async([&] { initialize(width, height, adaptation_sets.front()); })
    };
    boost::wait_for_all(futures.begin(), futures.end());
}

video_adaptation_set& mpd_parser::video_set(int column, int row) const {
    boost::const_multi_array_ref<video_adaptation_set, 2> matrix_ref{
        video_adaptation_sets_.data(), boost::extents[width][height] };
    boost::array<decltype(matrix_ref)::index, 2> index{ column,row };  //!order
    return const_cast<video_adaptation_set&>(matrix_ref(index));
}

audio_adaptation_set& mpd_parser::audio_set() const {
    return *audio_adaptation_set_;
}

