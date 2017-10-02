#pragma once

#include <boost/asio/io_service.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using Task = std::function<void()>;

class Executor
{
    struct ThreadData
    {
        std::thread t;
        boost::asio::io_service *io_service = nullptr;
        std::exception_ptr eptr;
    };

    using Threads = std::vector<ThreadData>;

public:
    bool throw_exceptions = true;
    bool silent = false;

public:
    Executor(size_t nThreads = std::thread::hardware_concurrency(), const std::string &name = "");
    Executor(const std::string &name, size_t nThreads = std::thread::hardware_concurrency());
    ~Executor();

    size_t numberOfThreads() const { return nThreads; }

    template <class T>
    void push(T &&t)
    {
        static_assert(!std::is_lvalue_reference<T>::value, "Argument cannot be an lvalue");

        Task task([t = std::move(t)]{ t(); });
        io_service.post(std::move(task));
    }

    void join();
    void stop();
    void wait();
    bool stopped() const { return io_service.stopped(); }

private:
    size_t nThreads = std::thread::hardware_concurrency();
    Threads thread_pool;
    boost::asio::io_service io_service;
    std::unique_ptr<boost::asio::io_service::work> work;
    std::atomic_bool wait_ = false;
    std::mutex m;
    std::condition_variable cv;

    void run(size_t i);
    void try_throw();
    void wait_stop();
    void set_thread_name(const std::string &name, size_t i) const;
};

size_t get_max_threads(size_t N = 4);
