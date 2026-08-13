module;
export module kittyImage;

import ftxui;
import std;
import std.compat;

using namespace ftxui;

enum class KittySupport {
    None,
    Local,
};

export class KittyPrinter {
public:
    static KittySupport get_kitty_support();
    static void printToRGBA(const std::vector<uint8_t> &img_data, int img_width, int img_height);
    static void printToRGBA(const std::vector<uint8_t> &img_data, int img_width, int img_height, int x, int y);
    static void printToRGBAwithCorner(const std::vector<uint8_t> &img_data, int img_width, int img_height, int corner);
    static int printToRGBAwithID(const std::vector<uint8_t> &img_data, int img_width, int img_height, int x, int y);
    static void printPNG(std::string_view img_path);
    static void deleteAll();
    static void deleteWithID(const int _id);
    static std::vector<uint8_t> imageUrl2Bytes(std::string_view url);

    static void add_rounded_corners_rgba(unsigned char *data, int width, int height, int radius);
    static bool is_in_corner(int x, int y, int w, int h, int r);
    static float calculate_corner_alpha(int x, int y, int w, int h, int r);
};

export class KittyImage {
public:
    KittyImage(int width, int height, std::vector<uint8_t> data);
    KittyImage(int width, int height, std::string_view url);

    std::vector<uint8_t> data_;
    int width_;
    int height_;
};
