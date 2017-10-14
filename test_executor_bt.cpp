/*
name: main

*/

#include <primitives/executor.h>

#include <boost/thread/future.hpp>

#include <chrono>
#include <thread>

#undef min
#undef max

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

TEST_CASE("Checking executor", "[executor]")
{
    using namespace std::literals::chrono_literals;

    {
        boost::basic_thread_pool tp;
        std::atomic_int v = 0;

        for (int i = 0; i < 100; i++)
            boost::async(tp, [&v] { std::this_thread::sleep_for(10ms); v++; });
        REQUIRE(v < 100);
        std::this_thread::sleep_for(500ms);
        REQUIRE(v == 100);
        {
            std::vector<boost::future<void>> fs;
            for (int i = 0; i < 100; i++)
                fs.emplace_back(boost::async(tp, [&v] { std::this_thread::sleep_for(1ms); v++; }));
            boost::wait_for_all(fs.begin(), fs.end());
            REQUIRE(v == 200);
        }
        {
            std::vector<boost::future<void>> fs;
            for (int i = 0; i < 100; i++)
                fs.emplace_back(boost::async(tp, [&v] { std::this_thread::sleep_for(1ms); v++; }));
            boost::wait_for_all(fs.begin(), fs.end());
            REQUIRE(v == 300);
        }
        {
            std::vector<boost::future<void>> fs;
            for (int i = 0; i < 100; i++)
                fs.emplace_back(boost::async(tp, [&v] { std::this_thread::sleep_for(1ms); v++; }));
            boost::wait_for_all(fs.begin(), fs.end());
            REQUIRE(v == 400);
        }
        tp.close();
        REQUIRE_THROWS(boost::async(tp, [&v] { std::this_thread::sleep_for(1ms); v++; }));
    }

    {
        boost::basic_thread_pool tp;
        auto f = boost::async(tp, [] {throw std::runtime_error("123"); });
        std::this_thread::sleep_for(100ms);
        REQUIRE_THROWS(f.get());
    }

    {
        boost::basic_thread_pool tp(1);
        auto f = boost::async(tp, [] {throw std::runtime_error("123"); });
        std::this_thread::sleep_for(100ms);
        REQUIRE_THROWS(f.get());
    }
}

int main(int argc, char **argv)
{
    Catch::Session().run(argc, argv);

    /*std::istringstream ss("x: a\nx2:\n\ta: b\n\tc: d");
    auto n = YAML::Load(ss);
    get_variety_and_iterate(n, [](auto &v) {}, [](auto &v) {});
    get_variety_and_iterate(yaml(), [](auto &v) {}, [](auto &v) {});

    std::map<String, String> m;
    get_string_map(n, "x2", m);*/

    return 0;
}
