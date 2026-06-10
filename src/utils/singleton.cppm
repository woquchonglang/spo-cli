module;
export module singleton;
export template <typename Derived> class Singleton {
public:
    static Derived &instance() {
        static Derived instance;
        return instance;
    }

    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    Singleton(Singleton &&) = delete;
    Singleton &operator=(Singleton &&) = delete;

private:
    Singleton() = default;
    friend Derived;
};
