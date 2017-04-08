#pragma once

#include <primitives/filesystem.h>
#include <primitives/optional.h>

namespace command
{

using Args = Strings;

struct Options
{
    struct Stream
    {
        bool capture = false;
        bool inherit = false;
        std::function<void(const String &)> action;
    };

    Stream out;
    Stream err;
};

struct Result
{
    int rc;
    String out;
    String err;

    void write(path p) const;
};

Result execute(const Args &args, const Options &options = Options());
Result execute_with_output(const Args &args, const Options &options = Options());
Result execute_and_capture(const Args &args, const Options &options = Options());

} // namespace command

optional<path> resolve_executable(const path &p);
optional<path> resolve_executable(const std::vector<path> &paths);
