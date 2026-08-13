module;
export module kittyImageComponent;

import ftxui;
import spotifyWebAPI;
import std;

using namespace ftxui;

export class KittyImageComponent : public ftxui::ComponentBase {
public:
    KittyImageComponent(std::shared_ptr<SpotifyData> data) : data_(data) {}

    Element OnRender() override;

private:
private:
    Box box_;
    std::shared_ptr<SpotifyData> data_;
    bool render{ false };
};

export Component image_view(std::shared_ptr<SpotifyData> data);
