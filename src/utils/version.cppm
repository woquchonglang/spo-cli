module;
#include <sys/socket.h>
export module version;
import std;
import asio;
import http.client;

namespace http = beast::http;
namespace ssl = asio::ssl;

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif


export class Version {
public:
    Version(asio::any_io_executor exec)
            : exec(std::move(exec)), ctx(ssl::context::tls_client) {
        ctx.set_default_verify_paths();
        ctx.set_verify_mode(ssl::verify_peer);
#ifdef GIT_VERSION
        local_version = GIT_VERSION;
#endif
    }

    ~Version() = default;

    asio::awaitable<void> syncRemoteVersion() {
        http::request<http::string_body> req{ http::verb::get, remote_target,
                                              11 };
        req.set(http::field::host, remote_host);

        auto res = co_await client_read(std::string(remote_host), "443", ctx, std::move(req));

        auto result = res.result_int();
        if (result == 302) {
            auto it = res.find(http::field::location);
            if (it != res.end()) {
                std::string location = it->value();
                auto pos = location.find_last_of('/');
                if (pos != std::string::npos) {
                    std::string tag = location.substr(pos + 2);
                    remote_version = tag;
                }
            }
            co_return;
        }
        co_return;
    }

    asio::awaitable<bool> isLatest() {
        if (!remote_version.has_value()) {
            co_await syncRemoteVersion();
        }
        if (!remote_version.has_value() || !local_version.has_value()) {
            co_return false;
        }
        auto split = [](const std::string &v) {
            std::vector<int> parts;
            std::stringstream ss(v);
            std::string seg;
            while (std::getline(ss, seg, '.')) {
                parts.push_back(std::stoi(seg));
            }
            return parts;
        };

        auto r = split(remote_version.value());
        auto l = split(local_version.value());

        size_t n = std::max(r.size(), l.size());
        r.resize(n, 0);
        l.resize(n, 0);

        co_return r >= l;
    }

    std::optional<std::string> get_local_version() { return local_version; }
    std::optional<std::string> get_remote_version() { return remote_version; }


private:
    std::optional<std::string> remote_version;
    std::optional<std::string> local_version;

    static constexpr std::string_view remote_host = "github.com";
    static constexpr std::string_view remote_target =
            "/woquchonglang/spo-cli/releases/latest";

    asio::any_io_executor exec;
    asio::ssl::context ctx;
};
