/*
name: main

*/

#include <primitives/command.h>
#include <primitives/filesystem.h>
#include <primitives/hash.h>

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

TEST_CASE("Checking hashes", "[hash]")
{
    path empty_file;
    path fox_file;
    path fox_point_file;

    write_file(empty_file = fs::temp_directory_path() / fs::unique_path(), "");
    write_file(fox_file = fs::temp_directory_path() / fs::unique_path(), "The quick brown fox jumps over the lazy dog");
    write_file(fox_point_file = fs::temp_directory_path() / fs::unique_path(), "The quick brown fox jumps over the lazy dog.");

    REQUIRE(sha256  (""s)  == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    REQUIRE(sha3_256(""s)  == "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
    REQUIRE(sha256  ("0"s) == "5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9");
    REQUIRE(sha3_256("0"s) == "f9e2eaaa42d9fe9e558a9b8ef1bf366f190aacaa83bad2641ee106e9041096e4");
    REQUIRE(sha3_256("5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9f9e2eaaa42d9fe9e558a9b8ef1bf366f190aacaa83bad2641ee106e9041096e40"s)
                                         == "539e660d5e7d3245469e151f0c106ae2ac108a681f5083ac61f52381766aff3c");
    REQUIRE(strong_file_hash(""s)        == "539e660d5e7d3245469e151f0c106ae2ac108a681f5083ac61f52381766aff3c");
    REQUIRE(strong_file_hash(empty_file) == "539e660d5e7d3245469e151f0c106ae2ac108a681f5083ac61f52381766aff3c");

    REQUIRE(sha256  ("The quick brown fox jumps over the lazy dog"s)   == "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
    REQUIRE(sha3_256("The quick brown fox jumps over the lazy dog"s)   == "69070dda01975c8c120c3aada1b282394e7f032fa9cf32f4cb2259a0897dfc04");
    REQUIRE(sha256  ("The quick brown fox jumps over the lazy dog43"s) == "57186baa9fc99c713e6523f3c1ccae283cc563798cbcad0b1b3ea6d48a824618");
    REQUIRE(sha3_256("The quick brown fox jumps over the lazy dog43"s) == "e351cc10a404769508f9720e13c9b564e6dc008ec01ff0a82cafa6af7d314ed4");
    REQUIRE(sha3_256("57186baa9fc99c713e6523f3c1ccae283cc563798cbcad0b1b3ea6d48a824618e351cc10a404769508f9720e13c9b564e6dc008ec01ff0a82cafa6af7d314ed443"s)
                                                                             == "d7dd023e2e8f7b9b5df806ddacfa7510fcd441202399c7896960876f17610fe6");
    REQUIRE(strong_file_hash("The quick brown fox jumps over the lazy dog"s) == "d7dd023e2e8f7b9b5df806ddacfa7510fcd441202399c7896960876f17610fe6");
    REQUIRE(strong_file_hash(fox_file)                                       == "d7dd023e2e8f7b9b5df806ddacfa7510fcd441202399c7896960876f17610fe6");

    REQUIRE(sha256  ("The quick brown fox jumps over the lazy dog."s)   == "ef537f25c895bfa782526529a9b63d97aa631564d5d789c2b765448c8635fb6c");
    REQUIRE(sha3_256("The quick brown fox jumps over the lazy dog."s)   == "a80f839cd4f83f6c3dafc87feae470045e4eb0d366397d5c6ce34ba1739f734d");
    REQUIRE(sha256  ("The quick brown fox jumps over the lazy dog.44"s) == "cec965223bd76824dce43cef80630b7d69ccfc0afdfa466c5dd30a3113be8574");
    REQUIRE(sha3_256("The quick brown fox jumps over the lazy dog.44"s) == "d8abefa1b02baa4f0e3aee2fda7f677a0c793a3c3ecad56643b8833f0b372465");
    REQUIRE(sha3_256("cec965223bd76824dce43cef80630b7d69ccfc0afdfa466c5dd30a3113be8574d8abefa1b02baa4f0e3aee2fda7f677a0c793a3c3ecad56643b8833f0b37246544"s)
                                                                              == "853af62ed82f1c9079c2a1ee3f28806a520dc48fb702091e8f375466d7c484c0");
    REQUIRE(strong_file_hash("The quick brown fox jumps over the lazy dog."s) == "853af62ed82f1c9079c2a1ee3f28806a520dc48fb702091e8f375466d7c484c0");
    REQUIRE(strong_file_hash(fox_point_file) == "853af62ed82f1c9079c2a1ee3f28806a520dc48fb702091e8f375466d7c484c0");

    fs::remove(empty_file);
    fs::remove(fox_file);
    fs::remove(fox_point_file);
}

TEST_CASE("Checking filesystem & command2", "[fs,cmd]")
{
    path dir = fs::temp_directory_path() / "cppąń-storage-寿星天文历的";
    REQUIRE_NOTHROW(fs::create_directories(dir));

#ifdef _WIN32
    auto p = dir / "1.bat";
    write_file(p, "echo hello world");

    Command c;
    c.program = p;
    REQUIRE(c.execute());

    auto pbad = dir / "2";
    c.program = pbad;
    REQUIRE_THROWS(c.execute());
    std::error_code ec;
    REQUIRE(!c.execute(ec));
#endif
}

int main(int argc, char **argv)
{
    setup_utf8_filesystem();

    Catch::Session().run(argc, argv);

    return 0;
}
