module;
module ui:progressGuage;

import ftxui;
import std;

using namespace ftxui;

static const std::string charset_horizontal[11] = {
    " ", " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█",
    // An extra character in case when the fuzzer manage to have:
    // int(9 * (limit - limit_int) = 9
    "█"
};

// NOLINTNEXTLINE
static const std::string charset_horizontal_microsoft[11] = {
    " ", " ", " ", " ", "▌", "▌", "▌", "█", "█", "█",
    // An extra character in case when the fuzzer manage to have:
    // int(9 * (limit - limit_int) = 9
    "█"
};

static const std::string charset_vertical[10] = {
    "█",
    "▇",
    "▆",
    "▅",
    "▄",
    "▃",
    "▂",
    "▁",
    " ",
    // An extra character in case when the fuzzer manage to have:
    // int(8 * (limit - limit_int) = 8
    " ",
};

class ProgressGauge : public Node {
public:
    ProgressGauge(float progress, Direction direction, Color fill_color,
                  Color empty_color, std::vector<std::string> charset = {})
            : progress_(progress)
            , direction_(direction)
            , fill_color_(fill_color)
            , empty_color_(empty_color)
            , charset_(std::move(charset)) {
        // This handle NAN correctly:
        if (!(progress_ > 0.F)) {
            progress_ = 0.F;
        }
        if (!(progress_ < 1.F)) {
            progress_ = 1.F;
        }
    }

    void ComputeRequirement() override {
        switch (direction_) {
        case Direction::Right:
        case Direction::Left:
            requirement_.flex_grow_x = 1;
            requirement_.flex_grow_y = 0;
            requirement_.flex_shrink_x = 1;
            requirement_.flex_shrink_y = 0;
            break;
        case Direction::Up:
        case Direction::Down:
            requirement_.flex_grow_x = 0;
            requirement_.flex_grow_y = 1;
            requirement_.flex_shrink_x = 0;
            requirement_.flex_shrink_y = 1;
            break;
        }
        requirement_.min_x = 1;
        requirement_.min_y = 1;
    }

    void Render(Screen &screen) override {
        switch (direction_) {
        case Direction::Right:
            RenderHorizontal(screen, /*invert=*/false);
            break;
        case Direction::Up:
            RenderVertical(screen, /*invert=*/false);
            break;
        case Direction::Left:
            RenderHorizontal(screen, /*invert=*/true);
            break;
        case Direction::Down:
            RenderVertical(screen, /*invert=*/true);
            break;
        }
    }

    void RenderHorizontal(Screen &screen, bool invert) {
        if (box_.y_min > box_.y_max)
            return;

        const std::string *charset;
        int full;
        if (charset_.empty()) {
            charset = Terminal::GetQuirks().BlockCharacters() ?
                              charset_horizontal :
                              charset_horizontal_microsoft;
            full = 9;
        } else {
            charset = charset_.data();
            full = static_cast<int>(charset_.size()) - 1;
        }

        const float progress = invert ? 1.F - progress_ : progress_;
        const auto limit = float(box_.x_min) +
                           progress * float(box_.x_max - box_.x_min + 1);
        const int limit_int = static_cast<int>(limit);

        for (int y = box_.y_min; y <= box_.y_max; y++) {
            int x = box_.x_min;

            while (x < limit_int) {
                auto &cell = screen.CellAt(x++, y);
                cell.character = charset[full];
                cell.foreground_color = fill_color_;
                cell.bold = true;
            }

            while (x <= box_.x_max) {
                auto &cell = screen.CellAt(x++, y);
                cell.character = charset[0];
                cell.foreground_color = empty_color_;
                cell.bold = false;
            }
        }

        if (invert) {
            Invert(screen);
        }
    }

    void RenderVertical(Screen &screen, bool invert) {
        if (box_.x_min > box_.x_max) {
            return;
        }

        const std::string *charset;
        int full;
        if (charset_.empty()) {
            charset = charset_vertical;
            full = 8;
        } else {
            charset = charset_.data();
            full = static_cast<int>(charset_.size()) - 1;
        }

        // Draw the progress bar vertically across the full allocated width:
        const float progress = invert ? progress_ : 1.F - progress_;
        const float limit = float(box_.y_min) +
                            progress * float(box_.y_max - box_.y_min + 1);
        const int limit_int = static_cast<int>(limit);

        for (int x = box_.x_min; x <= box_.x_max; x++) {
            int y = box_.y_min;
            while (y < limit_int) {
                screen.at(x, y++) = charset[full];
            }
            if (y <= box_.y_max) {
                screen.at(x, y++) = charset[int(full * (limit - limit_int))];
            }
            while (y <= box_.y_max) {
                screen.at(x, y++) = charset[0];
            }
        }
        if (invert) {
            Invert(screen);
        }
    }

    void Invert(Screen &screen) {
        for (int y = box_.y_min; y <= box_.y_max; y++) {
            for (int x = box_.x_min; x <= box_.x_max; x++) {
                screen.CellAt(x, y).inverted ^= true;
            }
        }
    }

private:
    float progress_;
    Direction direction_;
    std::vector<std::string> charset_;
    Color fill_color_;
    Color empty_color_;
};

Element progressGaugeCharset(float progress,
                                    std::vector<std::string> charset,
                                    Color fill_color, Color empty_color,
                                    Direction direction = Direction::Right) {
    return std::make_shared<ProgressGauge>(progress, direction, fill_color,
                                           empty_color, std::move(charset));
}
