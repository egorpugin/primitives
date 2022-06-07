// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/source.h>
#include <primitives/yaml.h>
#include <primitives/sw/main.h>

#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#define REQUIRE_TIME(e, t)                                                                       \
    do                                                                                           \
    {                                                                                            \
        auto t1 = get_time([&] { REQUIRE(e); });                                                 \
        if (t1 > t)                                                                              \
        {                                                                                        \
            auto d = std::chrono::duration_cast<std::chrono::duration<float>>(t1 - t).count();   \
            REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " #e + std::to_string(d))); \
        }                                                                                        \
    } while (0)

#define REQUIRE_NOTHROW_TIME(e, t)  \
    if (get_time([&] { (e); }) > t) \
    REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " #e))

#define REQUIRE_NOTHROW_TIME_MORE(e, t) \
    if (get_time([&] { (e); }) <= t)    \
    REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " #e))

TEST_CASE("Checking source", "[source]")
{
    using namespace primitives::source;

    String in_yaml = R"(
source:
    git:
        url: https://github.com/egorpugin/primitives
        branch: master
)";

    String in_json = R"(
{ "source": { "git": {
    "url": "https://github.com/egorpugin/primitives",
    "tag": "{v}"
}}}
)";

    {
        auto s = Source::load(YAML::Load(in_yaml)["source"]);
        CHECK(dynamic_cast<Git*>(s.get()));
        CHECK_FALSE(dynamic_cast<Hg*>(s.get()));

        auto t1 = s->print();
        CHECK(t1 == R"(git:
"url": https://github.com/egorpugin/primitives
"branch": master
)");

        auto t2 = s->printKv();
        CHECK(t2[0].second == "git");
        CHECK(t2[1].second == "https://github.com/egorpugin/primitives");
        CHECK(t2[2].second == "master");

        nlohmann::json j;
        s->save(j);
        CHECK(j.dump() == R"({"git":{"branch":"master","url":"https://github.com/egorpugin/primitives"}})");

        yaml root;
        s->save(root);
        CHECK(YAML::Dump(root) == R"(git:
  url: https://github.com/egorpugin/primitives
  branch: master)");
    }

    {
        auto s = Source::load(nlohmann::json::parse(in_json)["source"]);
        CHECK(dynamic_cast<Git *>(s.get()));
        CHECK_FALSE(dynamic_cast<Hg *>(s.get()));

        {
            auto t1 = s->print();
            CHECK(t1 == R"(git:
"url": https://github.com/egorpugin/primitives
"tag": {v}
)");

            auto t2 = s->printKv();
            CHECK(t2[0].second == "git");
            CHECK(t2[1].second == "https://github.com/egorpugin/primitives");
            CHECK(t2[2].second == "{v}");

            nlohmann::json j;
            s->save(j);
            CHECK(j.dump() == R"({"git":{"tag":"{v}","url":"https://github.com/egorpugin/primitives"}})");

            yaml root;
            s->save(root);
            CHECK(YAML::Dump(root) == R"(git:
  url: https://github.com/egorpugin/primitives
  tag: "{v}")");
        }

        s->apply([](auto &&s) { return boost::replace_all_copy(s, "{v}", "1.2.3"); });

        {
            auto t1 = s->print();
            CHECK(t1 == R"(git:
"url": https://github.com/egorpugin/primitives
"tag": 1.2.3
)");

            auto t2 = s->printKv();
            CHECK(t2[0].second == "git");
            CHECK(t2[1].second == "https://github.com/egorpugin/primitives");
            CHECK(t2[2].second == "1.2.3");

            nlohmann::json j;
            s->save(j);
            CHECK(j.dump() == R"({"git":{"tag":"1.2.3","url":"https://github.com/egorpugin/primitives"}})");

            yaml root;
            s->save(root);
            CHECK(YAML::Dump(root) == R"(git:
  url: https://github.com/egorpugin/primitives
  tag: 1.2.3)");
        }
    }

    {
        CHECK_THROWS(Hg("url", "", "", "", 1));
        CHECK_NOTHROW(Hg("https://url", "", "", "", 1));
    }
}

int main(int argc, char *argv[])
{
    auto r = Catch::Session().run(argc, argv);
    return r;
}
