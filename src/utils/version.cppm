module;
#include <sys/socket.h>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/use_awaitable.hpp>
export module version;
import std;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

#ifndef GIT_VERSION
#define GIT_VERSION "unknown"
#endif


export class Version {
public:
    Version(net::any_io_executor exec) : exec(std::move(exec)) {
#ifdef GIT_VERSION
        local_version = GIT_VERSION;
#endif
    }

    ~Version() = default;

    net::awaitable<void> syncRemoteVersion() {
        ssl::context ctx(ssl::context::tlsv13_client);
        ctx.set_default_verify_paths();

        auto executor = co_await net::this_coro::executor;
        beast::ssl_stream<beast::tcp_stream> stream(executor, ctx);

        tcp::resolver resolver(executor);
        auto const results = co_await resolver.async_resolve(
                remote_host, "443", net::use_awaitable);

        co_await beast::get_lowest_layer(stream).async_connect(
                results, net::use_awaitable);
        co_await stream.async_handshake(ssl::stream_base::client,
                                        net::use_awaitable);

        http::request<http::string_body> req{ http::verb::get, remote_target,
                                              11 };
        req.set(http::field::host, remote_host);

        co_await http::async_write(stream, req, net::use_awaitable);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        co_await http::async_read(stream, buffer, res, net::use_awaitable);

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

    net::awaitable<bool> isLatest() {
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

    net::any_io_executor exec;
};
