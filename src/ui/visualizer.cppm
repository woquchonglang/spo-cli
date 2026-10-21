module;
#include "librespot.rs.h"
#include "cavacore.h"
module ui:visualizer;

import std;
import rwlock;

class Visualizer {
public:
    Visualizer(unsigned int rate = librespot::get_sample_rate(),
               int channels = librespot::get_num_channels(),
               int number_of_bars = number_of_bars_, int autosens = autosens_,
               double noise_reduction = noise_reduction_,
               int low_cut_off = low_cut_off_,
               int high_cut_off = high_cut_off_);
    ~Visualizer();
    void register_cb();

    cava_plan *plan;
    RwLock<std::vector<double>> bars;

    static constexpr int number_of_bars_ = 20;
    static constexpr int autosens_ = 1;
    static constexpr double noise_reduction_ = 0.77;
    static constexpr int low_cut_off_ = 50;
    static constexpr int high_cut_off_ = 10000;
};

std::unique_ptr<Visualizer> g_visualizer = nullptr;

extern "C" {
using CavaSinkCallback = void (*)(const double *samples, int per_channel,
                                  int channels);
void cava_bridge_register(CavaSinkCallback cb);
}

void cava_sink_cb(const double *samples, int per_channel, int channels) {
    auto guard = g_visualizer->bars.write();
    cava_execute(const_cast<double *>(samples), per_channel, guard.value.data(),
                 g_visualizer->plan);
}

Visualizer::Visualizer(unsigned int rate, int channels, int number_of_bars,
                       int autosens, double noise_reduction, int low_cut_off,
                       int high_cut_off) {
    plan = cava_init(number_of_bars, rate, channels, autosens, noise_reduction,
                     low_cut_off, high_cut_off, CAVA_SCALING_LINEAR);
    if (plan->status != 0) {
        std::println("visualizer error: cava init error: {}", plan->status);
    }
    bars.write().value.resize(number_of_bars * channels, 0.0);
}


Visualizer::~Visualizer() { cava_destroy(plan); }

void Visualizer::register_cb() { cava_bridge_register(cava_sink_cb); }
