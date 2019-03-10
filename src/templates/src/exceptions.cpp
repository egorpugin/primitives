// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/exceptions.h>
#ifdef USE_STACKTRACE
#include <boost/stacktrace.hpp>
#endif

#include <sstream>

namespace sw
{

BaseException::BaseException(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : file(file), function(function), line(line), message(msg)
{
#ifdef USE_STACKTRACE
    if (stacktrace)
    {
        boost::stacktrace::stacktrace t(4 /* skip */, -1 /* till the end */);
        std::ostringstream ss;
        ss << t;
        message += "\nStacktrace:\n" + ss.str();
    }
#endif
}

const char *BaseException::getMessage() const
{
    if (what_.empty())
        what_ = format();
    return what_.c_str();
}

std::string BaseException::format() const
{
    std::string s;
    //s += ex_type;
#ifdef USE_STACKTRACE
    //s += "Exception: " + message; // disabled for now, too poor :(
    s += "Exception in file " + file + ":" + std::to_string(line) + ", function " + function + ": " + message;
#else
    s += "Exception in file " + file + ":" + std::to_string(line) + ", function " + function + ": " + message;
#endif
    return s;
}

Exception::Exception(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException(file, function, line, msg, stacktrace), std::exception(getMessage())
{
}

RuntimeError::RuntimeError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException(file, function, line, msg, stacktrace), std::runtime_error(getMessage())
{
}

} // namespace sw
