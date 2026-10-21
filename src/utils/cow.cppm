module;
export module utils.cow;
import std;

export template <typename T> class Cow {
    std::atomic<std::shared_ptr<T>> ptr;

public:
    void write(std::shared_ptr<T> new_ptr) { ptr.store(std::move(new_ptr)); }

    std::shared_ptr<const T> read() const { return ptr.load(); }
};
