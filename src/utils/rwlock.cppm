module;
export module rwlock;

import std;

export template <typename T> class RwLock {
public:
    struct ReadGuard {
        std::shared_lock<std::shared_mutex> lock;
        const T &value;
    };

    struct WriteGuard {
        std::unique_lock<std::shared_mutex> lock;
        T &value;
    };

    explicit RwLock(T value) : value_(std::move(value)) {}

    ReadGuard read() const { return { std::shared_lock(mutex_), value_ }; }

    WriteGuard write() { return { std::unique_lock(mutex_), value_ }; }

private:
    mutable std::shared_mutex mutex_;
    T value_;
};
