module;
export module config;

import std;

export class Config {
public:
    Config();
    ~Config();

    std::string client_id;
    std::string client_secret;
    std::string login_redirect_url;
};

