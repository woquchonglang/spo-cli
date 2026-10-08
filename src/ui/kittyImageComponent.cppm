module;
export module kittyImageComponent;

import ftxui;
import spotify_data;
import std;

using namespace ftxui;

export class KittyImageComponent : public ftxui::ComponentBase {
public:
    KittyImageComponent(const ::Image &data) : data(data) {}

    Element OnRender() override;
    void clear();

private:
private:
    Box box_;
    const ::Image &data;
    bool render{ false };
};

export Component image_view(const ::Image &data);
