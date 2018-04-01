#pragma once

#include <boost/asio/io_service.hpp>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

using Task = std::function<void()>;

class Executor
{
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
    std::vector<std::thread> thread_pool;
    boost::asio::io_service io_service;
    std::unique_ptr<boost::asio::io_service::work> work;
    std::exception_ptr eptr;
    std::atomic_bool wait_ = false;
    std::mutex m;
    std::condition_variable cv;
    std::atomic_bool had_exception = false;
    std::unordered_map<std::thread::id, size_t> thread_ids;

    void run(size_t i);
    void try_throw();
    void wait_stop();
    void set_thread_name(const std::string &name) const;
};

size_t get_max_threads(size_t N = 4);
