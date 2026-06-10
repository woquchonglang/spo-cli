module;
export module timer;

import std;
import singleton;

struct Task {
    std::chrono::steady_clock::time_point time;
    std::function<void()> func;
};


export class Timer : public Singleton<Timer> {
public:
    void start() {
        is_running = true;
        event_loop_thread = std::jthread([this]() {
            std::unique_lock<std::mutex> lock(mutex_);
            while (is_running) {
                cv_.wait(lock, [this]() { return !is_running || !tasks.empty(); });
                auto now = std::chrono::steady_clock::now();
                while (!tasks.empty() && tasks.front().time <= now) {
                    auto task = tasks.front().func;
                    tasks.erase(tasks.begin());
                    lock.unlock();
                    task();
                    lock.lock();
                }
                if (!tasks.empty()) {
                    cv_.wait_until(lock, tasks.front().time);
                }
            }
        });
    }

    void stop() {
        is_running = false;
        cv_.notify_all();
    }

    void add_task(std::chrono::steady_clock::time_point time, std::function<void()> func) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.push_back({ time, func });
        std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) { return a.time < b.time; });
        cv_.notify_all();
    }

    void add_task_after(std::chrono::milliseconds delay, std::function<void()> func) {
        add_task(std::chrono::steady_clock::now() + delay, func);
    }

    ~Timer() { stop(); }

private:
    Timer() : is_running(false) {}
    friend class Singleton<Timer>;

    std::atomic<bool> is_running;
    std::jthread event_loop_thread;
    std::condition_variable cv_;
    std::mutex mutex_;
    std::vector<Task> tasks;
};
