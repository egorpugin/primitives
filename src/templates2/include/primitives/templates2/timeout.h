#pragma once

#include <chrono>
#include <stdexcept>

namespace primitives::date_time {

using namespace std::literals;

struct timeout_exception : ::std::runtime_error { using ::std::runtime_error::runtime_error; };

void sleep(auto &&d) {
    std::this_thread::sleep_for(d);
}

inline auto make_timeout(
    std::chrono::duration<float> start = 10ms,
    std::chrono::duration<float> max = 100ms,
    std::chrono::duration<float> step = 10ms,
    std::chrono::duration<float> exception_timeout = 20s) {
    struct timeout {
        std::chrono::duration<float> t;
        std::chrono::duration<float> max;
        std::chrono::duration<float> step;
        std::chrono::duration<float> exception_timeout;
        std::chrono::duration<float> total;

        int n{0}; // can't make lambda because we need n access, need we?

        explicit operator bool() const { return total > exception_timeout; }
        void sleep(auto &&t) {
            total += t;
            std::this_thread::sleep_for(t);
            if (*this) {
                throw timeout_exception{std::format("timeout: {}", total)};
            }
        }
        auto operator()() {
            simple_wait();
            t += step;
            t = std::min(max, t);
            ++n;
            return operator bool();
        }
        void simple_wait() {
            sleep(t);
        }
    };
    return timeout{start, max, step, exception_timeout};
}

template <typename Exception>
auto while_no_exception_with_timeout(auto &&f, auto && ... args) {
    auto timeout = make_timeout(args...);
    while (1) {
        try {
            return f();
        }
        catch (Exception &e) {
            LOG_WARN(logger, e.what());
            timeout();
        }
    }
}
auto while_no_exception_with_timeout(auto &&f, auto && ... args) {
    return while_no_exception_with_timeout<std::exception>(f, args...);
}

} // namespace primitives::templates
