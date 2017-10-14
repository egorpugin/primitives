#pragma once

#define BOOST_THREAD_PROVIDES_EXECUTORS
#define BOOST_THREAD_PROVIDES_VARIADIC_THREAD
#define BOOST_THREAD_VERSION 5

#include <boost/thread/executor.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>

boost::generic_executor_ref get_executor()
{
    static boost::basic_thread_pool tp;
    return boost::generic_executor_ref(tp);
}

