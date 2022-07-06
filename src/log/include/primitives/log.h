// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

//#include <something>

#pragma once

#ifndef DISABLE_LOGGER

#include <boost/format.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#define LOGGER_GLOBAL_INIT
#define LOGGER_GLOBAL_DESTROY
//#define LOGGER_CONFIGURE(loglevel, filename) initLogger(loglevel, filename)

#define CREATE_LOGGER(name) const char *name
#define GET_LOGGER(module) module
#define INIT_LOGGER(name, module) name = GET_LOGGER(module)
#define DECLARE_LOGGER(name, module) CREATE_LOGGER(name)(GET_LOGGER(module))
#define DECLARE_STATIC_LOGGER(name, module) static DECLARE_LOGGER(name, module)
#define DECLARE_STATIC_LOGGER2(name) DECLARE_STATIC_LOGGER(name##_logger, #name)

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

namespace primitives::log {

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(thread_id, "ThreadID", boost::log::attributes::current_thread_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)

#ifdef _WIN32
//static auto in_mode = 0x01;
static auto out_mode = 0x02;
static auto app_mode = 0x08;
//static auto binary_mode = 0x20;
#else
//static auto in_mode = std::ios::in;
static auto out_mode = std::ios::out;
static auto app_mode = std::ios::app;
//static auto binary_mode = std::ios::binary;
#endif

typedef boost::log::sinks::text_file_backend tfb;
typedef boost::log::sinks::synchronous_sink<tfb> sfs;

inline boost::shared_ptr<tfb> backend;
inline boost::shared_ptr<tfb> backend_debug;
inline boost::shared_ptr<tfb> backend_trace;

inline boost::shared_ptr<
    boost::log::sinks::synchronous_sink<
    boost::log::sinks::text_ostream_backend
    >> c_log;

void logFormatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
{
    static boost::thread_specific_ptr<boost::posix_time::time_facet> tss(0);
    if (!tss.get())
    {
        tss.reset(new boost::posix_time::time_facet);
        tss->set_iso_extended_format();
    }
    strm.imbue(std::locale(strm.getloc(), tss.get()));

    std::string s;
    boost::log::formatting_ostream ss(s);
    ss << "[" << rec[severity] << "]";

    strm << "[" << rec[timestamp] << "] " <<
        boost::format("[%08x] %-9s %s")
        % rec[thread_id]
        % s
        % rec[boost::log::expressions::smessage];
}

void logFormatterSimple(boost::log::record_view const& rec, boost::log::formatting_ostream& strm)
{
    strm << boost::format("%s")
        % rec[boost::log::expressions::smessage];
}

} // namespace primitives::log

struct LoggerSettings
{
    std::string log_level = "DEBUG";
    std::string log_file;
    bool simple_logger = false;
    bool print_trace = false;
    bool append = false;
};

void initLogger(LoggerSettings &s)
{
    try
    {
        bool disable_log = s.log_level == "";

        boost::algorithm::to_lower(s.log_level);

        boost::log::trivial::severity_level level;
        std::stringstream(s.log_level) >> level;

        boost::log::trivial::severity_level trace;
        std::stringstream("trace") >> trace;

        if (!disable_log)
        {
            auto c_log = boost::log::add_console_log();
            primitives::log::c_log = c_log;
            if (s.simple_logger)
                c_log->set_formatter(&primitives::log::logFormatterSimple);
            else
                c_log->set_formatter(&primitives::log::logFormatter);
            c_log->set_filter(boost::log::trivial::severity >= level);
        }

        if (s.log_file != "")
        {
            auto open_mode = primitives::log::out_mode;
            if (s.append)
                open_mode = primitives::log::app_mode;

            // input
            if (level > boost::log::trivial::severity_level::trace)
            {
                auto backend = boost::make_shared<primitives::log::tfb>
                    (
                        boost::log::keywords::file_name = s.log_file + ".log." + s.log_level + ".%N.txt",
                        boost::log::keywords::rotation_size = 10 * 1024 * 1024,
                        boost::log::keywords::open_mode = open_mode//,
                                                                   //boost::log::keywords::auto_flush = true
                        );
                primitives::log::backend = backend;

                auto sink = boost::make_shared<primitives::log::sfs>(backend);
                if (s.simple_logger)
                    sink->set_formatter(&primitives::log::logFormatterSimple);
                else
                    sink->set_formatter(&primitives::log::logFormatter);
                sink->set_filter(boost::log::trivial::severity >= level);
                boost::log::core::get()->add_sink(sink);
            }

            if (level == boost::log::trivial::severity_level::trace || s.print_trace)
            {
                auto add_logger = [&s](auto severity, const auto &name, auto &g_backend)
                {
                    auto backend = boost::make_shared<primitives::log::tfb>
                        (
                            boost::log::keywords::file_name = s.log_file + ".log." + name + ".%N.txt",
                            boost::log::keywords::rotation_size = 10 * 1024 * 1024,
                            // always append to trace, do not recreate
                            boost::log::keywords::open_mode = primitives::log::app_mode//,
                                                                                //boost::log::keywords::auto_flush = true
                            );
                    g_backend = backend;

                    auto sink_trace = boost::make_shared<primitives::log::sfs>(backend);
                    // trace to file always has complex format
                    sink_trace->set_formatter(&primitives::log::logFormatter);
                    sink_trace->set_filter(boost::log::trivial::severity >= severity);
                    boost::log::core::get()->add_sink(sink_trace);
                };

                add_logger(boost::log::trivial::severity_level::debug, "debug", primitives::log::backend_debug);
                add_logger(boost::log::trivial::severity_level::trace, "trace", primitives::log::backend_trace);
            }
        }
        boost::log::add_common_attributes();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(logger, "logger initialization failed with exception " << e.what() << ", will use default logger settings");
    }
}

void loggerFlush()
{
    if (primitives::log::c_log)
        primitives::log::c_log->flush();
    if (primitives::log::backend)
        primitives::log::backend->flush();
    if (primitives::log::backend_trace)
        primitives::log::backend_trace->flush();
}

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
