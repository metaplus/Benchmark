#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <boost/filesystem.hpp>

namespace
{
    using namespace std::literals;
    namespace filesystem = boost::filesystem;
    const filesystem::path in_path[] = {
        "d:/DrivingInCountry_test_903frames.yuv",
        "e:/DrivingInCountry_test_903frames.yuv" };
    constexpr size_t kbyte = 1024;
    constexpr size_t kbyte512 = kbyte * 512;
    constexpr size_t mbyte = kbyte * 1024;
    constexpr size_t mbyte4 = mbyte * 4;
    constexpr size_t mbyte16 = mbyte * 16;
    constexpr size_t mbyte64 = mbyte * 64;
    constexpr size_t mbyte128 = mbyte * 128;
    constexpr size_t mbyte256 = mbyte * 256;

}

#pragma warning(push)
#pragma warning(disable:4244)
class time_guard
{
    std::chrono::steady_clock::time_point time_mark_;
public:
    time_guard() :time_mark_{ std::chrono::steady_clock::now() } {}
    ~time_guard()
    {
        using namespace std::chrono;
        std::cout << std::hex << '@'
            << std::this_thread::get_id() << ' '
            << std::divides<double>{}(
                duration_cast<milliseconds>(
                    steady_clock::now() - time_mark_).count(), 1000)
            << " secs" << std::endl;
    }
};    
#pragma warning(pop)

//msvc stl implementation use std::decay_t<Args...> and compressed pair
//both allocated on stack, cache friendly
template<typename Lambda,typename... Args>
class callable :Lambda
{   //private inherited from lambda expression
    template<size_t... Index>
    decltype(auto) invoke_impl(std::index_sequence<Index...>)
    {
        return static_cast<Lambda>(*this)(
            std::forward<Args>(std::get<Index>(args_))...);
    }
    std::tuple<Args...> args_;
public:
    explicit callable(Lambda&& func, Args&&... args) :
        Lambda{ std::forward<Lambda>(func) },
        args_{ std::forward<Args>(args)... } {}
   // std::result_of_t<Lambda(Args...)> 
    decltype(auto) operator()() {
        std::cout << "operator()" << std::endl;
        return invoke_impl(std::index_sequence_for<Args...>{});
    }
};

template<typename Lambda, typename... Args>
decltype(auto) make_callable(Lambda&& func, Args&&... args)
{
    return callable<Lambda, Args...>(
        std::forward<Lambda>(func), 
        std::forward<Args>(args)...);
}


int main()
{
    assert(filesystem::is_regular_file(in_path[0]) && filesystem::is_regular_file(in_path[1]));
    assert(filesystem::file_size(in_path[0]) == filesystem::file_size(in_path[1]));
    std::cout
        << "file name\t" << in_path[0].filename() << '\n'
        << "file size\t" << file_size(in_path[0]) << std::endl;
    for (auto const stride : {kbyte512, mbyte, mbyte4, mbyte16, mbyte64, mbyte128, mbyte256})
    {
        std::cout << std::dec
            << "1 disk reader 1 std::thread\n"
            << "block stride " << stride << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        time_guard time_counter;
        std::atomic<int> frame_count = 0;
        std::atomic<size_t> count_byte = 0;
        auto task = [&](int index) {
            std::ifstream ifs{ in_path[index].string() ,std::ios::binary };
            assert(ifs);
            std::vector<char> buffer(stride);
            do
            {
                count_byte.fetch_add(ifs.gcount(), std::memory_order_relaxed);
                if (ifs.eof()) break;
                auto pos = frame_count.fetch_add(1, std::memory_order_acquire);
                ifs.seekg(pos*buffer.size(), std::ios::beg);
            } while (ifs.read(buffer.data(), buffer.size()));
            count_byte.fetch_add(ifs.gcount(), std::memory_order_relaxed);

        };
        std::thread reader0{ task,0 };
        std::thread reader1{ task,1 };
        reader0.join(); reader1.join();
        std::cout << std::dec << "count all\t" << count_byte << std::endl;
    }
}
