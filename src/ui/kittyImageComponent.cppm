module;
#include <sys/ioctl.h>
#include <stb_image_resize2.h>
export module ui:kittyImageComponent;

import ftxui;
import std;
import spotify.data;
import :kittyImage;

using namespace ftxui;

export class KittyImageComponent : public ftxui::ComponentBase {
public:
    KittyImageComponent(const ::Image &data);

    Element OnRender() override;
    void clear();

private:
    Box box_;
    Box last_box_;
    const ::Image &data;
    bool render{ false };
    bool layout_done_{ false };

    int ws_col;
    int ws_row;
};

KittyImageComponent::KittyImageComponent(const ::Image &data) : data(data) {
    winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);
    ws_col = sz.ws_col;
    ws_row = sz.ws_row;
}

Element KittyImageComponent::OnRender() {
    winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);

    auto cell_width = sz.ws_xpixel / sz.ws_col;
    auto cell_height = sz.ws_ypixel / sz.ws_row;

    // TODO: move image instead of delete and re-render
    if (ws_col != sz.ws_col || ws_row != sz.ws_row) {
        KittyPrinter::deleteAll();
        ws_col = sz.ws_col;
        ws_row = sz.ws_row;
        layout_done_ = false;
        render = false;
    }

    if (data.width) {
        const int dst_w = 388, dst_h = 388;
        Element placeholder;

        // handle 640x640 image (spotify large image)
        if (data.width == 640 && data.height == 640) {
            placeholder = text("") | size(WIDTH, EQUAL, dst_w / cell_width) |
                          size(HEIGHT, EQUAL, dst_h / cell_height) |
                          reflect(box_);
        } else {
            placeholder = text("") |
                          size(WIDTH, EQUAL, data.width / cell_width) |
                          size(HEIGHT, EQUAL, data.height / cell_height) |
                          reflect(box_);
        }
        if (box_ != last_box_) {
            last_box_ = box_;
            render = false;
            clear(); // TODO: move image instead of delete and re-render
        }
        if (!layout_done_) {
            layout_done_ = true;
            return placeholder;
        }
        if (!render) {
            std::cout << "\033[" << box_.y_min + 1 << ";" << box_.x_min + 1
                      << "H"; // kitty image 从光标位置开始渲染
            KittyImage image(data.width, data.height, data.url);
            // handle 640x640 image (spotify large image)
            if (image.width_ == 640 && image.height_ == 640) {
                auto rgba_data = KittyPrinter::rawToRGBA(image.data_);
                const int channels = 4;
                std::vector<uint8_t> resized_data(static_cast<size_t>(dst_w) *
                                                  dst_h * channels);
                unsigned char *ret = stbir_resize_uint8_linear(
                        rgba_data.data(), image.width_, image.height_, 0,
                        resized_data.data(), dst_w, dst_h, 0, STBIR_RGBA);
                if (!ret) {
                    return placeholder;
                }
                KittyPrinter::printRGBA(resized_data, dst_w, dst_h, 15);
            } else {
                KittyPrinter::printToRGBAwithCorner(image.data_, image.width_,
                                                    image.height_, 15);
            }
            render = true;
        }
        return placeholder;
    } else {
        auto placeholder = text("") | reflect(box_);
        return placeholder;
    }
}

void KittyImageComponent::clear() { KittyPrinter::deleteAll(); }

export Component image_view(const ::Image &data) {
    return std::make_shared<KittyImageComponent>(data);
}
