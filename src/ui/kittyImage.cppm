module;
#include "base64.hpp"
#include "stb_image.h"
#include <sys/socket.h>
#include <boost/url.hpp>
export module ui:kittyImage;

import ftxui;
import std;
import std.compat;
import httplib;

using namespace ftxui;

enum class KittySupport {
    None,
    Local,
};

export class KittyPrinter {
public:
    static KittySupport get_kitty_support();
    static std::vector<uint8_t> rawToRGBA(const std::vector<uint8_t> &raw_data);
    static void printToRGBA(const std::vector<uint8_t> &img_data, int img_width,
                            int img_height);
    static void printToRGBA(const std::vector<uint8_t> &img_data, int img_width,
                            int img_height, int x, int y);
    static void printToRGBAwithCorner(const std::vector<uint8_t> &img_data,
                                      int img_width, int img_height,
                                      int corner);
    static int printToRGBAwithID(const std::vector<uint8_t> &img_data,
                                 int img_width, int img_height, int x, int y);
    static void printPNG(std::string_view img_path);
    static void printRGBA(const std::vector<uint8_t> &rgba_data, int width,
                          int height, int corner);
    static void deleteAll();
    static void deleteWithID(const int _id);
    static std::vector<uint8_t> imageUrl2Bytes(std::string_view url);

    static void add_rounded_corners_rgba(unsigned char *data, int width,
                                         int height, int radius);
    static bool is_in_corner(int x, int y, int w, int h, int r);
    static float calculate_corner_alpha(int x, int y, int w, int h, int r);
};

namespace fs = std::filesystem;

class ImageCache {
public:
    ImageCache() { fs::create_directories(dir_); }

    std::optional<std::vector<uint8_t>> getFromCache(const std::string &key) {
        fs::path file = path_for(key);
        if (!fs::exists(file))
            return std::nullopt;
        return read_file(file);
    }

    void saveToCache(const std::string &key, const std::vector<uint8_t> &body) {
        fs::path file = path_for(key);
        fs::path tmp = file;
        tmp += ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary);
            if (!out)
                return;
            out.write(reinterpret_cast<const char *>(body.data()),
                      static_cast<std::streamsize>(body.size()));
        }
        std::error_code ec;
        fs::rename(tmp, file, ec);
        if (ec)
            fs::remove(tmp, ec);
    }

private:
    fs::path dir_ = fs::path(getenv("HOME")) / ".config" / "spo-cli" /
                    ".cache" / "images";

    fs::path path_for(const std::string &key) const {
        std::hash<std::string> h;
        auto hash = h(key);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%016zx", static_cast<size_t>(hash));
        return dir_ / (std::string(buf) + ".img");
    }

    static std::optional<std::vector<uint8_t>> read_file(const fs::path &file) {
        std::ifstream in(file, std::ios::binary | std::ios::ate);
        if (!in)
            return std::nullopt;
        auto size = in.tellg();
        if (size < 0)
            return std::nullopt;
        in.seekg(0);
        std::vector<uint8_t> data(static_cast<size_t>(size));
        in.read(reinterpret_cast<char *>(data.data()), size);
        if (!in)
            return std::nullopt;
        return data;
    }
};

export class KittyImage {
public:
    KittyImage(int width, int height, std::vector<uint8_t> data);
    KittyImage(int width, int height, const std::string &url);

    std::vector<uint8_t> data_;
    int width_;
    int height_;
};

static std::atomic<int> numbers{ 0 };
static ImageCache cache;

// https://sw.kovidgoyal.net/kitty/glossary/#envvar-TERM
KittySupport KittyPrinter::get_kitty_support() {
    static bool is_kitty = false;
    const char *term = std::getenv("TERM");
    if (!term)
        return KittySupport::None;
    is_kitty = (strstr(term, "xterm-kitty") != nullptr);
    return is_kitty ? KittySupport::Local : KittySupport::None;
}

static Dimensions get_terminal_size() { return Terminal::Size(); }

std::vector<uint8_t> KittyPrinter::imageUrl2Bytes(std::string_view url) {
    auto parsed = boost::urls::parse_uri(url);
    if (!parsed) {
        std::cerr << "Invalid URL: " << url << std::endl;
        return {};
    }
    boost::urls::url_view _url = *parsed;

    httplib::Client cli(_url.host());
    cli.set_address_family(AF_INET);

    std::string path(_url.encoded_path());
    if (_url.has_query()) {
        path += "?" + std::string(_url.encoded_query());
    }
    auto res = cli.Get(path);
    if (res && res->status == 200) {
        return std::vector<uint8_t>(res->body.begin(), res->body.end());
    } else {
        std::cerr << "Failed to fetch image: " << res.error() << std::endl;
    }
    return {};
}

namespace fs = std::filesystem;
class TempFile {
public:
    TempFile() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 9999);

        fs::path temp_dir = fs::temp_directory_path();
        path_ = temp_dir /
                (TEMP_FILE_PREFIX + std::to_string(dis(gen)) + ".tmp");
    }

    // https://sw.kovidgoyal.net/kitty/graphics-protocol/#the-transmission-medium
    // when t=t, kitty will read the file and delete it immediately
    ~TempFile() = default;

    bool write(const std::vector<uint8_t> &data) {
        std::ofstream file(path_, std::ios::binary);
        if (!file)
            return false;
        file.write(reinterpret_cast<const char *>(data.data()), data.size());
        return true;
    }

    std::string path() const { return path_.string(); }

private:
    fs::path path_;
    std::string TEMP_FILE_PREFIX = "tty-graphics-protocol.spo.";
};

std::vector<uint8_t>
KittyPrinter::rawToRGBA(const std::vector<uint8_t> &raw_data) {
    int w = 0, h = 0;
    auto rgba = stbi_load_from_memory(raw_data.data(), raw_data.size(), &w, &h,
                                      nullptr, 4);
    if (!rgba) {
        return {};
    }
    std::vector<uint8_t> rgba_data(rgba, rgba + w * h * 4);
    stbi_image_free(rgba);
    return rgba_data;
}

void KittyPrinter::printToRGBA(const std::vector<uint8_t> &img_data,
                               int img_width, int img_height) {
    auto rgba_data = rawToRGBA(img_data);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height
              << ",a=T,t=t;" << base64_path << "\x1b\\";
    std::cout.flush();
}

bool parse_response(const std::string &response, int temp_number,
                    int &real_id) {
    std::regex pattern(R"(Gi=(\d+),I=(\d+);)");
    std::smatch matches;

    if (std::regex_search(response, matches, pattern)) {
        int id = std::stoul(matches[1].str());
        int number = std::stoul(matches[2].str());

        if (number == temp_number) {
            real_id = id;
            return true;
        }
    }

    return false;
}

void KittyPrinter::printToRGBA(const std::vector<uint8_t> &img_data,
                               int img_width, int img_height, int x, int y) {
    auto rgba = stbi_load_from_memory(img_data.data(), img_data.size(),
                                      &img_width, &img_height, nullptr, 4);
    std::vector<uint8_t> rgba_data(rgba, rgba + img_width * img_height * 4);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height
              << ",w=" << img_width << ",h=" << img_height << ",a=T,t=t;"
              << base64_path << "\x1b\\";
    std::cout.flush();
    stbi_image_free(rgba);
}

void KittyPrinter::printToRGBAwithCorner(const std::vector<uint8_t> &img_data,
                                         int img_width, int img_height,
                                         int corner) {
    auto rgba_data = rawToRGBA(img_data);
    add_rounded_corners_rgba(rgba_data.data(), img_width, img_height, corner);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height
              << ",w=" << img_width << ",h=" << img_height << ",a=T,t=t;"
              << base64_path << "\x1b\\";
    std::cout.flush();
}

int KittyPrinter::printToRGBAwithID(const std::vector<uint8_t> &img_data,
                                    int img_width, int img_height, int x,
                                    int y) {
    auto rgba = stbi_load_from_memory(img_data.data(), img_data.size(),
                                      &img_width, &img_height, nullptr, 4);
    std::vector<uint8_t> rgba_data(rgba, rgba + img_width * img_height * 4);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height
              << ",x=" << x << ",y=" << y << ",I=" << numbers++ << ",a=T,t=t;"
              << base64_path << "\x1b\\";
    std::cout.flush();
    std::string response;
    char ch;
    int id = 0;
    while (1) {
        if (std::cin.get(ch)) {
            response += ch;
            if (response.size() >= 2 &&
                response[response.size() - 2] == '\x1b' &&
                response[response.size() - 1] == '\\') {
                parse_response(response, numbers, id);
                break;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    stbi_image_free(rgba);
    return id;
}

void KittyPrinter::printPNG(std::string_view img_path) {
    auto base64_path = base64::to_base64(img_path);
    std::cout << "\x1b_Gf=100,a=T,t=f;" << base64_path << "\x1b\\";
    std::cout.flush();
}

void KittyPrinter::printRGBA(const std::vector<uint8_t> &rgba_data, int width,
                             int height, int corner) {
    std::vector<uint8_t> copy =
            rgba_data; // add_rounded_corners will modify the data, so we make a copy
    add_rounded_corners_rgba(copy.data(), width, height, corner);
    TempFile temp;
    temp.write(copy);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << width << ",v=" << height << ",w=" << width
              << ",h=" << height << ",a=T,t=t;" << base64_path << "\x1b\\";
    std::cout.flush();
}

void KittyPrinter::deleteAll() {
    numbers = 0;
    std::cout << "\x1b_Ga=d;\x1b\\";
    std::cout.flush();
}

void KittyPrinter::deleteWithID(const int _id) {
    if (numbers > 0)
        numbers--;
    std::cout << "\x1b_Ga=d,d=i,i=" << _id << ";\x1b\\";
    std::cout.flush();
}

KittyImage::KittyImage(int width, int height, std::vector<uint8_t> data) {
    width_ = width;
    height_ = height;
    data_ = std::move(data);
}

KittyImage::KittyImage(int width, int height, const std::string &url) {
    width_ = width;
    height_ = height;
    auto _cache = cache.getFromCache(url);
    if (_cache.has_value() && _cache.value().size() > 0) {
        data_ = _cache.value();
    } else {
        data_ = KittyPrinter::imageUrl2Bytes(url);
        cache.saveToCache(url, data_);
    }
}

void KittyPrinter::add_rounded_corners_rgba(unsigned char *data, int width,
                                            int height, int radius) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (is_in_corner(x, y, width, height, radius)) {
                float alpha =
                        calculate_corner_alpha(x, y, width, height, radius);
                int idx = (y * width + x) * 4;
                data[idx + 3] = (unsigned char)(alpha * 255);
            }
        }
    }
}

bool KittyPrinter::is_in_corner(int x, int y, int w, int h, int r) {
    if (x < r && y < r) {
        return (x - r) * (x - r) + (y - r) * (y - r) > r * r;
    }
    if (x > w - r && y < r) {
        return (x - (w - r)) * (x - (w - r)) + (y - r) * (y - r) > r * r;
    }
    if (x < r && y > h - r) {
        return (x - r) * (x - r) + (y - (h - r)) * (y - (h - r)) > r * r;
    }
    if (x > w - r && y > h - r) {
        return (x - (w - r)) * (x - (w - r)) + (y - (h - r)) * (y - (h - r)) >
               r * r;
    }
    return false;
}

float KittyPrinter::calculate_corner_alpha(int x, int y, int w, int h, int r) {
    float dx = 0, dy = 0;

    if (x < r && y < r) {
        dx = x - r;
        dy = y - r;
    } else if (x > w - r && y < r) {
        dx = x - (w - r);
        dy = y - r;
    } else if (x < r && y > h - r) {
        dx = x - r;
        dy = y - (h - r);
    } else if (x > w - r && y > h - r) {
        dx = x - (w - r);
        dy = y - (h - r);
    } else {
        return 1.0f;
    }

    float dist = std::sqrtf(dx * dx + dy * dy);
    float alpha = 1.0f - (dist / r);
    return alpha < 0 ? 0 : (alpha > 1 ? 1 : alpha);
}
