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

bool gUseStackTrace;
std::string gSymbolPath;

#ifdef USE_STACKTRACE
#ifdef _WIN32
#include "exceptions_msvc.h"
#endif
#endif

namespace sw
{

namespace detail
{

BaseException::BaseException(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : file(file), function(function), line(line), message(msg)
{
#ifdef USE_STACKTRACE
    if (stacktrace && gUseStackTrace)
    {
        boost::stacktrace::stacktrace t(3 /* skip */, -1 /* till the end */);
        message += "\nStacktrace:\n";
#ifdef _WIN32
        message += ::to_string(t);
#else
        std::ostringstream ss;
        ss << t;
        message += ss.str();
#endif
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

}

Exception::Exception(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException(file, function, line, msg, stacktrace), std::exception(
#ifdef _MSC_VER
        getMessage()
#endif
    )
{
}

RuntimeError::RuntimeError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException/*Exception*/(file, function, line, msg, stacktrace), std::runtime_error(getMessage())
{
}

/*RuntimeError::RuntimeError(const RuntimeError &rhs)
    : Exception(rhs), std::runtime_error(rhs)
{
}*/

} // namespace sw
