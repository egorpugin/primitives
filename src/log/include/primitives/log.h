// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

//#include <something>

#pragma once

#ifndef DISABLE_LOGGER

#include <boost/log/trivial.hpp>

#define LOGGER_GLOBAL_INIT
#define LOGGER_GLOBAL_DESTROY
#define LOGGER_CONFIGURE(loglevel, filename) initLogger(loglevel, filename)

#define CREATE_LOGGER(name) const char *name
#define GET_LOGGER(module) module
#define INIT_LOGGER(name, module) name = GET_LOGGER(module)
#define DECLARE_LOGGER(name, module) CREATE_LOGGER(name)(GET_LOGGER(module))
#define DECLARE_STATIC_LOGGER(name, module) static DECLARE_LOGGER(name, module)

#ifdef LOGGER_PRINT_MODULE
#define LOG_BOOST_LOG_MESSAGE(logger, message) "[" << logger << "] " << message
#else
#define LOG_BOOST_LOG_MESSAGE(logger, message) message
#endif
#define LOG_TRACE(logger, message) BOOST_LOG_TRIVIAL(trace) << LOG_BOOST_LOG_MESSAGE(logger, message)
#define LOG_DEBUG(logger, message) BOOST_LOG_TRIVIAL(debug) << LOG_BOOST_LOG_MESSAGE(logger, message)
#define LOG_INFO(logger, message) BOOST_LOG_TRIVIAL(info) << LOG_BOOST_LOG_MESSAGE(logger, message)
#define LOG_WARN(logger, message) BOOST_LOG_TRIVIAL(warning) << LOG_BOOST_LOG_MESSAGE(logger, message)
#define LOG_ERROR(logger, message) BOOST_LOG_TRIVIAL(error) << LOG_BOOST_LOG_MESSAGE(logger, message)
#define LOG_FATAL(logger, message) BOOST_LOG_TRIVIAL(fatal) << LOG_BOOST_LOG_MESSAGE(logger, message)

#define LOG_FLUSH() loggerFlush()

struct LoggerSettings
{
    std::string log_level = "DEBUG";
    std::string log_file;
    bool simple_logger = false;
    bool print_trace = false;
    bool append = false;
};

void initLogger(LoggerSettings &s);
void loggerFlush();

#else // !USE_LOGGER

#define LOGGER_GLOBAL_INIT
#define LOGGER_GLOBAL_DESTROY
#define LOGGER_CONFIGURE(loglevel, filename)

#define CREATE_LOGGER(name)
#define GET_LOGGER(module)
#define INIT_LOGGER(name, module)
#define DECLARE_LOGGER(name, module)
#define DECLARE_STATIC_LOGGER(name, module)

//#define LOG(logger, level, message)
#define LOG_TRACE(logger, message)
#define LOG_DEBUG(logger, message)
#define LOG_INFO(logger, message)
#define LOG_WARN(logger, message)
#define LOG_ERROR(logger, message)
#define LOG_FATAL(logger, message)

#define LOG_FLUSH()

#define IS_LOG_TRACE_ENABLED(logger)
#define IS_LOG_DEBUG_ENABLED(logger)
#define IS_LOG_INFO_ENABLED(logger)
#define IS_LOG_WARN_ENABLED(logger)
#define IS_LOG_ERROR_ENABLED(logger)
#define IS_LOG_FATAL_ENABLED(logger)

#endif
