module;
#include <sys/ioctl.h>
module kittyImageComponent;

import ftxui;
import kittyImage;
import std;

Component image_view(const ::Image &data) {
    return std::make_shared<KittyImageComponent>(data);
}

Element KittyImageComponent::OnRender() {
    winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);

    auto cell_width = sz.ws_xpixel / sz.ws_col;
    auto cell_height = sz.ws_ypixel / sz.ws_row;

    if (data.width) {
        auto placeholder = text("") | size(WIDTH, EQUAL, 300 / cell_width) |
                           size(HEIGHT, EQUAL, 300 / cell_height) |
                           reflect(box_);
        if (!render) {
            std::cout << "\033[" << box_.y_min + 1 << ";" << box_.x_min
                      << "H"; // kitty image 从光标位置开始渲染
            KittyImage image(data.width, data.height, data.url);
            KittyPrinter::printToRGBAwithCorner(image.data_, image.width_,
                                                image.height_, 10);
            render = true;
        }
        // else if (render && !focus) {
        //         KittyPrinter::deleteAll();
        //     }
        return placeholder;
    } else {
        auto placeholder = text("") | reflect(box_);
        return placeholder;
    }
}

void KittyImageComponent::clear() { KittyPrinter::deleteAll(); }
