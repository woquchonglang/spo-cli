module;
#include <unistd.h>
#include <boost/asio.hpp>
module proc;

import event;

using namespace Proc;

CpuInfo::CpuInfo() : usage(0) {
    update_time();
    last_utime = utime;
    last_stime = stime;
    clk_tck = sysconf(_SC_CLK_TCK);

    next_tick = std::chrono::steady_clock::now();
    last_tp = next_tick;
}

double CpuInfo::calc(unsigned long long u0, unsigned long long u1,
                     unsigned long long s0, unsigned long long s1,
                     double wall_sec) {
    unsigned long long delta_jiff = (u1 - u0) + (s1 - s0);
    double cpu_sec = (double)delta_jiff / clk_tck;
    // long logical_core_count = sysconf(_SC_NPROCESSORS_ONLN);
    // double pct = cpu_sec / wall_sec * 100 / logical_core_count; // Solaris mode
    double pct = cpu_sec / wall_sec * 100; // Irix mode
    return pct;
}

void CpuInfo::update_time() {
    std::ifstream f("/proc/self/stat");
    std::string token;
    for (int i = 0; i < 13; i++)
        f >> token;
    f >> utime >> stime;
}

void CpuInfo::update_usage() {
    update_time();
    auto curr_tp = std::chrono::steady_clock::now();
    double wall_sec = std::chrono::duration<double>(curr_tp - last_tp).count();
    usage = calc(last_utime, utime, last_stime, stime, wall_sec);

    last_utime = utime;
    last_stime = stime;
    last_tp = curr_tp;
    next_tick +=
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(T);

    // std::println("cpu_time: {:.2f}%, utime: {}, stime: {}", usage, utime,
    //              stime);
}

double CpuInfo::get_usage() const { return usage; }

StatmInfo::StatmInfo()
        : virt_mb(0), rss_mb(0), share_mb(0) {

        };

void StatmInfo::update_memory() {
    std::ifstream f("/proc/self/statm");
    if (!f.is_open())
        return;

    long size_page, resident_page, share_page;
    f >> size_page >> resident_page >> share_page;

    long page_size = sysconf(_SC_PAGESIZE); // bytes per page

    auto page2mb = [page_size](long pages) -> double {
        // pages * page_size → bytes → /1024/1024 → MB
        return static_cast<double>(pages) * page_size / (1024.0 * 1024.0);
    };

    virt_mb = page2mb(size_page);
    rss_mb = page2mb(resident_page);
    share_mb = page2mb(share_page);

    // std::println("mem:{:.2f}M", rss_mb);
}

double StatmInfo::get_memory() const { return rss_mb; }

Info::Info() : cpu_info(CpuInfo{}), stat_info(StatmInfo{}) {}

boost::asio::awaitable<void> Info::sample(boost::asio::io_context &ioc) {
    boost::asio::steady_timer timer(ioc);
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto next = cpu_info.read().value.next_tick;
        if (next > now) {
            timer.expires_at(next);
            co_await timer.async_wait(boost::asio::use_awaitable);
        }
        cpu_info.write().value.update_usage();
        stat_info.write().value.update_memory();
        eventQueue.enqueue(SPOCLI::Event::Refresh);
    }
}
