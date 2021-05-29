#pragma once

#include <condition_variable>
#include <mutex>

namespace primitives::templates {

struct cv_event {
    explicit operator bool() const {
        return b;
    }

    void set() {
        {
            std::unique_lock lk(m);
            b = true;
        }
        cv.notify_all();
    }
    void wait() {
        std::unique_lock lk(m);
        cv.wait(lk, [this] { return b; });
    }

private:
    bool b = false;
    std::condition_variable cv;
    std::mutex m;
};

template <typename T, typename Mutex = std::recursive_mutex>
struct thread_safe {
private:
    struct proxy {
        proxy(thread_safe &g)
            : lk{ g.m }
            , value{ &g.value }
        {}
        T *operator->() const {
            return value;
        }
        T &operator*() const {
            return *value;
        }
    private:
        std::unique_lock<Mutex> lk;
        T *value;
    };

public:
    const proxy operator->() {
        return *this;
    }
    proxy operator*() /*requires requires (T x) {
        x[std::declval<typename T::key_type>()];
    }*/ {
        return *this;
    }
private:
    Mutex m;
    T value;
};

} // namespace primitives::templates
