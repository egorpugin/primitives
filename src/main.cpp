// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/command.h>
#include <primitives/date_time.h>
#include <primitives/executor.h>
#include <primitives/filesystem.h>
#include <primitives/hash.h>

#include <boost/algorithm/string.hpp>

#include <chrono>
#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#ifdef __clang__
#define REQUIRE_TIME(e, t)
#define REQUIRE_NOTHROW_TIME(e, t)
#define REQUIRE_NOTHROW_TIME_MORE(e, t)
#else
#define REQUIRE_TIME(e, t)                                                                         \
    do                                                                                             \
    {                                                                                              \
        auto t1 = get_time([&] { CHECK(e); });                                                   \
        if (t1 > t)                                                                                \
        {                                                                                          \
            auto d = std::chrono::duration_cast<std::chrono::duration<float>>(t1 - t).count();     \
            REQUIRE_NOTHROW(throw std::runtime_error("time exceed on "## #e + std::to_string(d))); \
        }                                                                                          \
    } while (0)

#define REQUIRE_NOTHROW_TIME(e, t)                 \
    if (get_time([&] { (e); }) > t) \
        REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " ## #e))

#define REQUIRE_NOTHROW_TIME_MORE(e, t)                 \
    if (get_time([&] { (e); }) <= t) \
        REQUIRE_NOTHROW(throw std::runtime_error("time exceed on " ## #e))
#endif

TEST_CASE("Checking hashes", "[hash]")
{
    path empty_file;
    path fox_file;
    path fox_point_file;

    write_file(empty_file = fs::temp_directory_path() / unique_path(), "");
    write_file(fox_file = fs::temp_directory_path() / unique_path(), "The quick brown fox jumps over the lazy dog");
    write_file(fox_point_file = fs::temp_directory_path() / unique_path(), "The quick brown fox jumps over the lazy dog.");

    CHECK(sha256  (""s)  == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha3_256(""s)  == "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
    CHECK(sha256  ("0"s) == "5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9");
    CHECK(sha3_256("0"s) == "f9e2eaaa42d9fe9e558a9b8ef1bf366f190aacaa83bad2641ee106e9041096e4");
    CHECK(sha3_256("5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9f9e2eaaa42d9fe9e558a9b8ef1bf366f190aacaa83bad2641ee106e9041096e40"s)
                                         == "539e660d5e7d3245469e151f0c106ae2ac108a681f5083ac61f52381766aff3c");
    CHECK(strong_file_hash(""s)        == "539e660d5e7d3245469e151f0c106ae2ac108a681f5083ac61f52381766aff3c");
    CHECK(strong_file_hash(empty_file) == "539e660d5e7d3245469e151f0c106ae2ac108a681f5083ac61f52381766aff3c");

    CHECK(sha256  ("The quick brown fox jumps over the lazy dog"s)   == "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
    CHECK(sha3_256("The quick brown fox jumps over the lazy dog"s)   == "69070dda01975c8c120c3aada1b282394e7f032fa9cf32f4cb2259a0897dfc04");
    CHECK(sha256  ("The quick brown fox jumps over the lazy dog43"s) == "57186baa9fc99c713e6523f3c1ccae283cc563798cbcad0b1b3ea6d48a824618");
    CHECK(sha3_256("The quick brown fox jumps over the lazy dog43"s) == "e351cc10a404769508f9720e13c9b564e6dc008ec01ff0a82cafa6af7d314ed4");
    CHECK(sha3_256("57186baa9fc99c713e6523f3c1ccae283cc563798cbcad0b1b3ea6d48a824618e351cc10a404769508f9720e13c9b564e6dc008ec01ff0a82cafa6af7d314ed443"s)
                                                                             == "d7dd023e2e8f7b9b5df806ddacfa7510fcd441202399c7896960876f17610fe6");
    CHECK(strong_file_hash("The quick brown fox jumps over the lazy dog"s) == "d7dd023e2e8f7b9b5df806ddacfa7510fcd441202399c7896960876f17610fe6");
    CHECK(strong_file_hash(fox_file)                                       == "d7dd023e2e8f7b9b5df806ddacfa7510fcd441202399c7896960876f17610fe6");

    CHECK(sha256  ("The quick brown fox jumps over the lazy dog."s)   == "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c");
    CHECK(sha3_256("The quick brown fox jumps over the lazy dog."s)   == "a80f839cd4f83f6c3dafc87feae470045e4eb0d366397d5c6ce34ba1739f734d");
    CHECK(sha256  ("The quick brown fox jumps over the lazy dog.44"s) == "cec965223bd76824dce43cef80630b7d69ccfc0afdfa466c5dd30a3113be8574");
    CHECK(sha3_256("The quick brown fox jumps over the lazy dog.44"s) == "d8abefa1b02baa4f0e3aee2fda7f677a0c793a3c3ecad56643b8833f0b372465");
    CHECK(sha3_256("cec965223bd76824dce43cef80630b7d69ccfc0afdfa466c5dd30a3113be8574d8abefa1b02baa4f0e3aee2fda7f677a0c793a3c3ecad56643b8833f0b37246544"s)
                                                                              == "853af62ed82f1c9079c2a1ee3f28806a520dc48fb702091e8f375466d7c484c0");

    CHECK(blake2b_512(""s) == "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce"s);
    CHECK(blake2b_512("The quick brown fox jumps over the lazy dog"s) == "a8add4bdddfd93e4877d2746e62817b116364a1fa7bc148d95090bc7333b3673f82401cf7aa2e4cb1ecd90296e3f14cb5413f8ed77be73045b13914cdcd6a918"s);
    CHECK(blake2b_512("The quick brown fox jumps over the lazy dog."s) == "87af9dc4afe5651b7aa89124b905fd214bf17c79af58610db86a0fb1e0194622a4e9d8e395b352223a8183b0d421c0994b98286cbf8c68a495902e0fe6e2bda2"s);
    CHECK(blake2b_512("The quick brown fox jumps over the lazy dog.44"s) == "818784daf74541cd168f9bde4e6c7e654945a504f3678505efb75636e07acc22341e78f06595d73a2e184ad8883264360f334f0a6095b4033b8496b4326f3695"s);
    CHECK(blake2b_512("d8abefa1b02baa4f0e3aee2fda7f677a0c793a3c3ecad56643b8833f0b372465818784daf74541cd168f9bde4e6c7e654945a504f3678505efb75636e07acc22341e78f06595d73a2e184ad8883264360f334f0a6095b4033b8496b4326f369544"s)
        == "38b66d21d113eb60e20941d2ff3aa38f5287f97045a3998be5cefae3686956a678afc7b92312d4013d925a50a03d6b57b42619f635445eb070fb42b4ff63a2ee"s);

    CHECK(strong_file_hash_blake2b_sha3("The quick brown fox jumps over the lazy dog."s) == "3_2$38b66d21d113eb60e20941d2ff3aa38f5287f97045a3998be5cefae3686956a678afc7b92312d4013d925a50a03d6b57b42619f635445eb070fb42b4ff63a2ee");
    CHECK(strong_file_hash_blake2b_sha3(fox_point_file) == "3_2$38b66d21d113eb60e20941d2ff3aa38f5287f97045a3998be5cefae3686956a678afc7b92312d4013d925a50a03d6b57b42619f635445eb070fb42b4ff63a2ee");

    CHECK(check_strong_file_hash("The quick brown fox jumps over the lazy dog."s, "3_2$38b66d21d113eb60e20941d2ff3aa38f5287f97045a3998be5cefae3686956a678afc7b92312d4013d925a50a03d6b57b42619f635445eb070fb42b4ff63a2ee"));
    CHECK(check_strong_file_hash(fox_point_file, "3_2$38b66d21d113eb60e20941d2ff3aa38f5287f97045a3998be5cefae3686956a678afc7b92312d4013d925a50a03d6b57b42619f635445eb070fb42b4ff63a2ee"));

    CHECK(strong_file_hash("The quick brown fox jumps over the lazy dog."s) == "853af62ed82f1c9079c2a1ee3f28806a520dc48fb702091e8f375466d7c484c0");
    CHECK(strong_file_hash(fox_point_file) == "853af62ed82f1c9079c2a1ee3f28806a520dc48fb702091e8f375466d7c484c0");

    fs::remove(empty_file);
    fs::remove(fox_file);
    fs::remove(fox_point_file);
}

TEST_CASE("Checking filesystem & command2", "[fs,cmd]")
{
    using namespace primitives;

    path dir = fs::temp_directory_path() / u"cppąń-storage-寿星天文历的";
    REQUIRE_NOTHROW(fs::create_directories(dir));

    error_code ec;

#ifdef _WIN32
    auto p = dir / "1.bat";
    write_file(p, "echo hello world");

    Command c;
    c.program = p;
    REQUIRE_NOTHROW(c.execute());

    auto pbad = dir / "2";
    c.program = pbad;
    REQUIRE_THROWS(c.execute());
    REQUIRE_NOTHROW(c.execute(ec));
    CHECK(ec);

    REQUIRE_NOTHROW(Command::execute(p));
    REQUIRE_THROWS(Command::execute(pbad));
    REQUIRE_NOTHROW(Command::execute(pbad, ec));
    CHECK(ec);
#endif

    REQUIRE_NOTHROW(Command::execute({ "cmake", "--version" }));
    REQUIRE_THROWS(Command::execute({ "cmake", "--versionx" }));
    REQUIRE_NOTHROW(Command::execute({ "cmake", "--versionx" }, ec));
    CHECK(ec);
    {
        Command c;
        c.program = "cmake";
        c.args.push_back("--version");
        c.out.file = "1.txt";
        REQUIRE_NOTHROW(c.execute());
        CHECK(fs::exists("1.txt"));
    }

#ifdef _WIN32
    {
        path x;
        error_code ec;

        // match
        write_file("x.exe", "");
        x = resolve_executable("x");
        CHECK(x == "x.exe");
        fs::remove("x.exe", ec);

        // register match
        write_file("x.ExE", "");
        x = resolve_executable("x");
        CHECK(x == "x.exe");
        fs::remove("x.exe", ec);

        // dir mismatch
        fs::create_directories("x.exe");
        x = resolve_executable("x");
        CHECK(x == "");
        fs::remove("x.exe", ec);

        // register dir mismatch
        fs::create_directories("x.eXe");
        x = resolve_executable("x");
        CHECK(x == "");
        fs::remove("x.exe", ec);

        // missing file
        x = resolve_executable("x123");
        CHECK(x == "");

        // missing file with ext
        x = resolve_executable("x123.exe");
        CHECK(x == "");

        // bad ext, non existing
        x = resolve_executable("x.txt");
        CHECK(x == "");

        // allow resolving of custom existing files
        write_file("x.txt", "");
        x = resolve_executable("x.txt");
        CHECK(x == "x.txt");
        fs::remove("x.txt", ec);

        // dir mismatch
        write_file("x.exe.bat", "");
        fs::create_directories("x.exe");
        x = resolve_executable("x.exe");
        CHECK(x == "x.exe.bat");
        fs::remove("x.exe.bat", ec);
        fs::remove("x.exe", ec);

        // other
        x = resolve_executable("x");
        CHECK(x == "");
        fs::create_directories("x");
        x = resolve_executable("x");
        CHECK(x == "");
        fs::remove("x", ec);
    }

    CHECK("c:/Program Files/LLVM/bin/clang++.exe" == normalize_path("c:/Program Files/LLVM/bin\\clang++.exe"));
    CHECK(L"c:/Program Files/LLVM/bin/clang++.exe" == wnormalize_path(L"c:/Program Files/LLVM/bin\\clang++.exe"));
#endif
}

TEST_CASE("Checking executor", "[executor]")
{
    using namespace std::literals::chrono_literals;

    {
        Executor e(1);
        e.wait();
    }

    {
        std::atomic_int v = 0;

        Executor e;
        for (int i = 0; i < 100; i++)
            e.push([&v] { std::this_thread::sleep_for(10ms); v++; });
        CHECK(v < 100);
        std::this_thread::sleep_for(1s);
        CHECK(v == 100);
        for (int i = 0; i < 100; i++)
            e.push([&v] { std::this_thread::sleep_for(1ms); v++; });
        e.wait();
        CHECK(v == 200);
        for (int i = 0; i < 100; i++)
            e.push([&v] { std::this_thread::sleep_for(1ms); v++; });
        e.wait();
        CHECK(v == 300);
        for (int i = 0; i < 100; i++)
            e.push([&v] { std::this_thread::sleep_for(1ms); v++; });
        e.wait();
        CHECK(v == 400);

        e.stop();
        for (int i = 0; i < 100; i++)
            e.push([&v] { std::this_thread::sleep_for(1ms); v++; });
        CHECK(v == 400);
    }

    {
        Executor e;
        e.push([] {throw std::runtime_error("123"); });
        std::this_thread::sleep_for(100ms);
        REQUIRE_NOTHROW(e.wait());
        REQUIRE_NOTHROW(e.stop());
    }

    {
        Executor e;
        auto f = e.push([] {throw std::runtime_error("123"); });
        std::this_thread::sleep_for(100ms);
        REQUIRE_THROWS(f.get());
        REQUIRE_NOTHROW(e.stop());
    }

    {
        Executor e;
        for (int i = 0; i < 100; i++)
            e.push([] { std::this_thread::sleep_for(10ms); });
        e.push([] {throw std::runtime_error("123"); });
        for (int i = 0; i < 100; i++)
            e.push([] { std::this_thread::sleep_for(10ms); });
        REQUIRE_NOTHROW(e.wait());
        REQUIRE_NOTHROW(e.stop());
    }

    {
        Executor e;
        for (int i = 0; i < 100; i++)
            e.push([] { std::this_thread::sleep_for(10ms); });
        auto f = e.push([] {throw std::runtime_error("123"); });
        for (int i = 0; i < 100; i++)
            e.push([] { std::this_thread::sleep_for(10ms); });
        REQUIRE_THROWS(f.get());
        REQUIRE_NOTHROW(e.stop());
    }

    {
        Executor e;
        auto f1 = e.push([] { std::this_thread::sleep_for(1000ms); throw std::runtime_error("1"); });
        auto f2 = e.push([] { std::this_thread::sleep_for(2000ms); throw std::runtime_error("2"); });
        REQUIRE_THROWS(f1.get());
        REQUIRE_THROWS(f2.get());
        REQUIRE_NOTHROW(e.join());
    }

    {
        // FIXME: not working for executor_io
        Executor e(1);
        e.push([&e] { e.stop(); });
        e.push([] { std::this_thread::sleep_for(100ms); throw std::runtime_error("2"); });
        REQUIRE_NOTHROW(e.join());
    }

    {
        // FIXME: not working for executor_io
        Executor e;
        e.push([&e] { e.stop(); });
        e.push([] { std::this_thread::sleep_for(100ms); throw std::runtime_error("2"); });
        REQUIRE_NOTHROW(e.join());
    }

    {
        // FIXME: not working for executor_io
        Executor e;
        e.push([&e] { std::this_thread::sleep_for(500ms); e.stop(); });
        e.push([] { throw std::runtime_error("2"); });
        REQUIRE_NOTHROW(e.join());
    }

    {
        Executor e(2);
        auto f1 = e.push([] { std::this_thread::sleep_for(3s); });
        e.push([&f1] { f1.get(); });
        std::this_thread::sleep_for(1s);
        auto f3 = e.push([]
        {
            std::this_thread::sleep_for(500ms);
            return 5;
        });
        REQUIRE_TIME(f3.get() == 5, 1s);
        e.wait();
    }

    {
        Executor e;
        auto f1 = e.push([] { std::this_thread::sleep_for(100ms); return 1; });
        auto f2 = e.push([] { std::this_thread::sleep_for(200ms); return 2.0; });
        auto f3 = e.push([] { std::this_thread::sleep_for(300ms); return 'c'; });
        auto f4 = e.push([] { std::this_thread::sleep_for(400ms); });
        REQUIRE_NOTHROW_TIME(f1.get(), 200ms);
        REQUIRE_NOTHROW_TIME(f2.get(), 300ms);
        REQUIRE_NOTHROW_TIME(f3.get(), 400ms);
        REQUIRE_NOTHROW_TIME(f4.get(), 500ms);
        REQUIRE_NOTHROW_TIME(waitAll(f1, f2, f3, f4), 50ms);
    }

    {
        Executor e;
        auto f1 = e.push([] { std::this_thread::sleep_for(100ms); return 1; });
        auto f2 = e.push([] { std::this_thread::sleep_for(200ms); return 2.0; });
        auto f3 = e.push([] { std::this_thread::sleep_for(300ms); return 'c'; });
        auto f4 = e.push([] { std::this_thread::sleep_for(400ms); });
        waitAll(f1, f2, f3, f4);
        REQUIRE_NOTHROW_TIME(f1.get(), 50ms);
        REQUIRE_NOTHROW_TIME(f2.get(), 50ms);
        REQUIRE_NOTHROW_TIME(f3.get(), 50ms);
        REQUIRE_NOTHROW_TIME(f4.get(), 50ms);
    }

    {
        Executor e;
        std::vector<Future<void>> fs;
        fs.push_back(e.push([] { std::this_thread::sleep_for(100ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(200ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(300ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(400ms); }));
        REQUIRE_NOTHROW_TIME(fs[0].get(), 200ms);
        REQUIRE_NOTHROW_TIME(fs[1].get(), 300ms);
        REQUIRE_NOTHROW_TIME(fs[2].get(), 400ms);
        REQUIRE_NOTHROW_TIME(fs[3].get(), 500ms);
        REQUIRE_NOTHROW_TIME(waitAll(fs), 50ms);
    }

    {
        Executor e;
        std::vector<Future<void>> fs;
        fs.push_back(e.push([] { std::this_thread::sleep_for(100ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(200ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(300ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(400ms); }));
        waitAll(fs);
        REQUIRE_NOTHROW_TIME(fs[0].get(), 50ms);
        REQUIRE_NOTHROW_TIME(fs[1].get(), 50ms);
        REQUIRE_NOTHROW_TIME(fs[2].get(), 50ms);
        REQUIRE_NOTHROW_TIME(fs[3].get(), 50ms);
    }

    {
        Executor e;
        std::vector<Future<void>> fs;
        fs.push_back(e.push([] { std::this_thread::sleep_for(400ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(200ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(300ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(100ms); }));
        REQUIRE_NOTHROW_TIME(waitAny(fs), 200ms);
    }

    {
        Executor e;
        std::vector<Future<void>> fs;
        fs.push_back(e.push([] { std::this_thread::sleep_for(400ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(250ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(300ms); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(100ms); }));
        auto f = whenAny(fs);
        CHECK(f.get() == 3);
    }

    {
        Executor e;
        std::vector<Future<void>> fs;
        fs.push_back(e.push([] { throw std::runtime_error("1"); }));
        fs.push_back(e.push([] { std::this_thread::sleep_for(250ms); }));
        fs.push_back(e.push([] { throw std::runtime_error("2"); }));
        fs.push_back(e.push([] { throw std::runtime_error("3"); }));
        auto ex = waitAndGetAllExceptions(fs);
        CHECK(ex.size() == 3);
    }

    {
        Executor e;
        auto f3 = e.push([] { std::this_thread::sleep_for(300ms); });
        auto f2 = e.push([] { std::this_thread::sleep_for(200ms); });
        auto f4 = e.push([] { std::this_thread::sleep_for(400ms); });
        auto f1 = e.push([] { std::this_thread::sleep_for(100ms); });
        REQUIRE_NOTHROW_TIME_MORE(waitAll(f1, f2, f3, f4), 200ms);
    }

    {
        Executor e;
        auto f3 = e.push([] { std::this_thread::sleep_for(300ms); });
        auto f2 = e.push([] { std::this_thread::sleep_for(200ms); });
        auto f4 = e.push([] { std::this_thread::sleep_for(400ms); });
        auto f1 = e.push([] { std::this_thread::sleep_for(100ms); });
        REQUIRE_NOTHROW_TIME(waitAll(f1, f2, f3, f4), 500ms);
    }

    {
        Executor e;
        auto f3 = e.push([] { std::this_thread::sleep_for(300ms); });
        auto f2 = e.push([] { std::this_thread::sleep_for(200ms); });
        auto f4 = e.push([] { std::this_thread::sleep_for(400ms); });
        auto f1 = e.push([] { std::this_thread::sleep_for(100ms); });
        REQUIRE_NOTHROW_TIME(waitAny(f1, f2, f3, f4), 250ms);
    }

    {
        Executor e;
        auto f1 = e.push([] { std::this_thread::sleep_for(100ms); });
        auto f2 = e.push([] { std::this_thread::sleep_for(200ms); });
        auto f3 = e.push([] { std::this_thread::sleep_for(300ms); });
        auto f4 = e.push([] { std::this_thread::sleep_for(400ms); });
        auto f = whenAny(f2, f1, f3, f4);
        CHECK(f.get() == 1);
    }

    {
        volatile int i = 2;

        Executor e(1);
        e
            .push([&] { i += 2; })
            .then([&] { i *= 2; })
            .get()
            ;
        CHECK(i == 8);
    }

    {
        volatile int i = 2;

        Executor e;
        e.push(
            [&] { i += 2; })
            .then([&] { i *= 2; })
            .get()
            ;
        CHECK(i == 8);

        e.push(
            [&] { i += 2; })
            .then([&] { i *= 2; })
            .then([&] { i *= 2; })
            .then([&] { i *= 2; })
            .then([&] { i *= 2; })
            .get()
            ;
        CHECK(i == 160);

        auto f = e.push(
            [&] { i += 2; })
            .then([&] { i += 2; })
            ;
        auto f2 = f.then([&] { i += 2; });
        f.get();
        f2.get();
        CHECK(i == 166);

        auto f3 = e.push(
            [&] { i += 2; })
            .then([&] { i += 2; })
            ;
        auto f4 = f.then([&] { i += 2; });
        f4.get();
        f3.get();
        CHECK(i == 172);
    }

    // test fast wait
    {
        volatile int i = 0;

        Executor e(1);
        e.push([&i] { std::this_thread::sleep_for(100ms); i = 1; });
        e.wait();
        CHECK(i == 1);
    }
}

TEST_CASE("Checking executor: test fast wait", "[executor]")
{
    using namespace std::literals::chrono_literals;

    volatile int i = 0;

    Executor e(1);
    e.push([&i] { i = 1; });
    e.wait();
    CHECK(i == 1);
}

int main(int argc, char **argv)
try
{
    setup_utf8_filesystem();

    Catch::Session().run(argc, argv);

    return 0;
}
catch (std::exception &e)
{
    std::cerr << e.what() << "\n";
    return 1;
}
catch (...)
{
    std::cerr << "unknown exception" << "\n";
    return 1;
}
