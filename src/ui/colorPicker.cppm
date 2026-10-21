export module ui:colorPicker;

import std;

using Color3f = std::array<float, 3>;

inline float dist2(const Color3f &a, const Color3f &b) {
    const float dr = a[0] - b[0];
    const float dg = a[1] - b[1];
    const float db = a[2] - b[2];
    return dr * dr + dg * dg + db * db;
}

inline std::vector<Color3f> collect_samples(const std::uint8_t *pixels, int w,
                                            int h, int channels,
                                            int max_samples) {
    const int total = w * h;
    if (total <= 0 || pixels == nullptr)
        return {};

    const int step = total <= max_samples ? 1 : total / max_samples;
    std::vector<Color3f> samples;
    samples.reserve((total + step - 1) / step);

    for (int s = 0; s < total; s += step) {
        const std::uint8_t *p = pixels + static_cast<std::size_t>(s) * channels;
        samples.push_back({
                static_cast<float>(p[0]),
                static_cast<float>(p[1]),
                static_cast<float>(p[2]),
        });
    }
    return samples;
}

inline std::vector<Color3f> init_kmeanspp(const std::vector<Color3f> &samples,
                                          int k, std::mt19937 &rng) {
    std::vector<Color3f> centers;
    centers.reserve(k);
    if (samples.empty() || k <= 0)
        return centers;

    std::uniform_int_distribution<int> pick_idx(
            0, static_cast<int>(samples.size()) - 1);
    centers.push_back(samples[pick_idx(rng)]);

    std::vector<float> min_dist(samples.size(), 0.0f);
    for (std::size_t i = 0; i < samples.size(); ++i)
        min_dist[i] = dist2(samples[i], centers[0]);

    while (static_cast<int>(centers.size()) < k) {
        float sum = 0.0f;
        for (float d : min_dist)
            sum += d;

        std::size_t next = 0;
        if (sum <= 0.0f) {
            next = 0;
        } else {
            std::uniform_real_distribution<float> pick_r(0.0f, sum);
            float r = pick_r(rng);
            float acc = 0.0f;
            for (std::size_t i = 0; i < min_dist.size(); ++i) {
                acc += min_dist[i];
                if (acc >= r) {
                    next = i;
                    break;
                }
            }
        }
        centers.push_back(samples[next]);

        const Color3f &c = centers.back();
        for (std::size_t i = 0; i < samples.size(); ++i) {
            const float d = dist2(samples[i], c);
            if (d < min_dist[i])
                min_dist[i] = d;
        }
    }
    return centers;
}

inline std::vector<int> lloyd(const std::vector<Color3f> &samples,
                              std::vector<Color3f> &centers, int max_iter) {
    const int n = static_cast<int>(samples.size());
    const int k = static_cast<int>(centers.size());
    std::vector<int> assign(n, 0);
    std::vector<int> counts(k, 0);

    for (int iter = 0; iter < max_iter; ++iter) {
        bool changed = false;

        for (int i = 0; i < n; ++i) {
            int best = 0;
            float best_d = dist2(samples[i], centers[0]);
            for (int c = 1; c < k; ++c) {
                const float d = dist2(samples[i], centers[c]);
                if (d < best_d) {
                    best_d = d;
                    best = c;
                }
            }
            if (assign[i] != best) {
                assign[i] = best;
                changed = true;
            }
        }
        if (!changed && iter > 0)
            break;

        std::vector<Color3f> sum(k, Color3f{ 0.0f, 0.0f, 0.0f });
        std::fill(counts.begin(), counts.end(), 0);
        for (int i = 0; i < n; ++i) {
            const int c = assign[i];
            sum[c][0] += samples[i][0];
            sum[c][1] += samples[i][1];
            sum[c][2] += samples[i][2];
            ++counts[c];
        }
        for (int c = 0; c < k; ++c) {
            if (counts[c] > 0) {
                centers[c][0] = sum[c][0] / counts[c];
                centers[c][1] = sum[c][1] / counts[c];
                centers[c][2] = sum[c][2] / counts[c];
            }
        }
    }
    return counts;
}

struct PixelData {
    std::uint8_t r = 0, g = 0, b = 0, a = 255;
};

constexpr int PALETTE_MAX = 16;

struct ColorPalette {
    std::array<PixelData, PALETTE_MAX> colors{};
    int count = 0;
};

inline void generate_kmeans_palette(ColorPalette *palette,
                                    const std::uint8_t *pixels, int w, int h,
                                    int channels, int k = 8,
                                    int max_samples = 2048) {
    palette->count = 0;
    if (palette == nullptr || k <= 0 || k > PALETTE_MAX)
        return;

    auto samples = collect_samples(pixels, w, h, channels, max_samples);
    if (samples.empty())
        return;

    if (static_cast<int>(samples.size()) <= k) {
        for (std::size_t i = 0; i < samples.size(); ++i) {
            palette->colors[i] = PixelData{
                static_cast<std::uint8_t>(samples[i][0]),
                static_cast<std::uint8_t>(samples[i][1]),
                static_cast<std::uint8_t>(samples[i][2]),
                255,
            };
        }
        palette->count = static_cast<int>(samples.size());
        return;
    }

    std::mt19937 rng(0x9E3779B9u);
    auto centers = init_kmeanspp(samples, k, rng);
    auto counts = lloyd(samples, centers, /*max_iter=*/20);

    std::vector<int> order(k);
    for (int i = 0; i < k; ++i)
        order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return counts[a] > counts[b]; });

    for (int i = 0; i < k; ++i) {
        const int c = order[i];
        if (counts[c] == 0)
            continue;
        palette->colors[palette->count++] = PixelData{
            static_cast<std::uint8_t>(std::lround(centers[c][0])),
            static_cast<std::uint8_t>(std::lround(centers[c][1])),
            static_cast<std::uint8_t>(std::lround(centers[c][2])),
            255,
        };
    }
}
