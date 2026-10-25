export module ui:theme;

import ftxui;
import config.data;
import :colorPicker;
import spotify.data;
import :kittyImage;

class ThemeManager {
public:
    ThemeManager(bool follow = true) : follow_picker(follow) {}
    void update(::Image data);

    bool follow_picker;
    config::Component_theme theme;
    ColorPalette palette;
};

void ThemeManager::update(::Image data) {
    KittyImage image(data.width, data.height, data.url);
    auto _data = KittyPrinter::rawToRGBA(image.data_);
    generate_kmeans_palette(&palette, _data.data(), 300, 300, 4, 8, 2048);
}
