module;
#include "base64.hpp"
#include "stb_image.h"
#include "skyr/url.hpp"
#include <sys/socket.h>
module kittyImage;

import ftxui;
import httplib;
import std;
import std.compat;

static std::atomic<int> numbers{ 0 };

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
    auto _url = skyr::url(url);

    httplib::Client cli(_url.host());
    cli.set_address_family(AF_INET);
    auto res = cli.Get(_url.pathname());
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
        path_ = temp_dir / (TEMP_FILE_PREFIX + std::to_string(dis(gen)) + ".tmp");
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

void KittyPrinter::printToRGBA(const std::vector<uint8_t> &img_data, int img_width, int img_height) {
    auto rgba = stbi_load_from_memory(img_data.data(), img_data.size(), &img_width, &img_height, nullptr, 4);
    std::vector<uint8_t> rgba_data(rgba, rgba + img_width * img_height * 4);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height << ",a=T,t=t;" << base64_path << "\x1b\\";
    std::cout.flush();
    stbi_image_free(rgba);
}

bool parse_response(const std::string &response, int temp_number, int &real_id) {
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

void KittyPrinter::printToRGBA(const std::vector<uint8_t> &img_data, int img_width, int img_height, int x, int y) {
    auto rgba = stbi_load_from_memory(img_data.data(), img_data.size(), &img_width, &img_height, nullptr, 4);
    std::vector<uint8_t> rgba_data(rgba, rgba + img_width * img_height * 4);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height << ",w=" << img_width << ",h=" << img_height
              << ",a=T,t=t;" << base64_path << "\x1b\\";
    std::cout.flush();
    stbi_image_free(rgba);
}

void KittyPrinter::printToRGBAwithCorner(const std::vector<uint8_t> &img_data, int img_width, int img_height,
                                         int corner) {
    auto rgba = stbi_load_from_memory(img_data.data(), img_data.size(), &img_width, &img_height, nullptr, 4);
    std::vector<uint8_t> rgba_data(rgba, rgba + img_width * img_height * 4);
    add_rounded_corners_rgba(rgba_data.data(), img_width, img_height, corner);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height << ",w=" << img_width << ",h=" << img_height
              << ",a=T,t=t;" << base64_path << "\x1b\\";
    std::cout.flush();
    stbi_image_free(rgba);
}

int KittyPrinter::printToRGBAwithID(const std::vector<uint8_t> &img_data, int img_width, int img_height, int x, int y) {
    auto rgba = stbi_load_from_memory(img_data.data(), img_data.size(), &img_width, &img_height, nullptr, 4);
    std::vector<uint8_t> rgba_data(rgba, rgba + img_width * img_height * 4);
    TempFile temp;
    temp.write(rgba_data);
    std::string base64_path = base64::to_base64(temp.path());
    std::cout << "\x1b_Gf=32,s=" << img_width << ",v=" << img_height << ",x=" << x << ",y=" << y << ",I=" << numbers++
              << ",a=T,t=t;" << base64_path << "\x1b\\";
    std::cout.flush();
    std::string response;
    char ch;
    int id = 0;
    while (1) {
        if (std::cin.get(ch)) {
            response += ch;
            if (response.size() >= 2 && response[response.size() - 2] == '\x1b' &&
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

KittyImage::KittyImage(int width, int height, std::string_view url) {
    width_ = width;
    height_ = height;
    data_ = KittyPrinter::imageUrl2Bytes(url);
}

void KittyPrinter::add_rounded_corners_rgba(unsigned char *data, int width, int height, int radius) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (is_in_corner(x, y, width, height, radius)) {
                float alpha = calculate_corner_alpha(x, y, width, height, radius);
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
        return (x - (w - r)) * (x - (w - r)) + (y - (h - r)) * (y - (h - r)) > r * r;
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
