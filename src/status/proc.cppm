module;
#include <boost/asio.hpp>
export module proc;

import std;
import rwlock;
import singleton;

export namespace Proc {

class StatmInfo {
public:
    StatmInfo();
    void update_memory();
    double get_memory() const;

private:
    double virt_mb;
    double rss_mb;
    double share_mb;
};

class CpuInfo {
public:
    CpuInfo();
    double calc(unsigned long long u0, unsigned long long u1,
                unsigned long long s0, unsigned long long s1, double wall_sec);
    void update_time();
    void update_usage();
    double get_usage() const;

    std::chrono::steady_clock::time_point next_tick;

private:
    static constexpr auto T = std::chrono::duration<double>(1.0);

    std::chrono::steady_clock::time_point last_tp;
    unsigned long long utime, stime;
    unsigned long long last_utime, last_stime;
    long clk_tck;
    double usage;
};

class Info : public Singleton<Info> {
public:
    RwLock<CpuInfo> cpu_info;
    RwLock<StatmInfo> stat_info;

    boost::asio::awaitable<void> sample(boost::asio::io_context &ioc);

private:
    Info();
    friend class Singleton<Info>;
};
}
