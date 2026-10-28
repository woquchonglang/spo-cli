module;
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <boost/asio/awaitable.hpp>
export module http.client;

import std;
import log;
export import asio;
export import beast;

namespace http = beast::http;
namespace ssl = asio::ssl;
using namespace std::literals::chrono_literals;

export asio::awaitable<http::response<http::dynamic_body>>
client_read(std::string host, std::string port, ssl::context &ctx,
            http::request<http::string_body> req) {
    auto executor = co_await asio::this_coro::executor;
    auto resolver = asio::net::tcp::resolver{ executor };
    auto stream = ssl::stream<beast::tcp_stream>{ executor, ctx };

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        auto ec = std::error_code(static_cast<int>(::ERR_get_error()),
                                  asio::error::get_ssl_category());
        Log::instance().error("SNI setup failed: {}", ec.message());
        co_return http::response<http::dynamic_body>{};
    }

    // Set the expected hostname in the peer certificate for verification
    stream.set_verify_callback(ssl::host_name_verification(host));

    // Look up the domain name
    auto const results =
            co_await resolver.async_resolve(host, port, asio::use_awaitable);

    // FIX: I can't use ipv6 address, because my net doesn't support ipv6,
    // so I need to filter the results to only use ipv4 addresses.
    std::vector<asio::net::tcp::endpoint> v4_endpoints;
    for (const auto &entry : results) {
        if (entry.endpoint().address().is_v4()) {
            v4_endpoints.push_back(entry.endpoint());
        }
    }

    if (v4_endpoints.empty()) {
        Log::instance().error("No IPv4 address found for {}", host);
        co_return http::response<http::dynamic_body>{};
    }

    // Make the connection on the IP address we get from a lookup
    auto [connect_ec, _] =
            co_await beast::get_lowest_layer(stream).async_connect(
                    v4_endpoints, asio::cancel_after(10s, asio::as_tuple));
    if (connect_ec) {
        Log::instance().error("connect failed: {}", connect_ec.message());
        co_return http::response<http::dynamic_body>{};
    }

    // Perform the SSL handshake
    auto [handshake_ec] = co_await stream.async_handshake(
            ssl::stream_base::client, asio::cancel_after(10s, asio::as_tuple));
    if (handshake_ec) {
        Log::instance().error("handshake failed: {}", handshake_ec.message());
        co_return http::response<http::dynamic_body>{};
    }

    // Send the HTTP request to the remote host
    co_await http::async_write(stream, req, asio::cancel_after(10s));

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    co_await http::async_read(stream, buffer, res, asio::use_awaitable);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(5));

    // Gracefully close the stream - do not threat every error as an exception!
    auto [ec] = co_await stream.async_shutdown(asio::as_tuple);

    // ssl::error::stream_truncated, also known as an SSL "short read",
    // indicates the peer closed the connection without performing the
    // required closing handshake (for example, Google does this to
    // improve performance). Generally this can be a security issue,
    // but if your communication protocol is self-terminated (as
    // it is with both HTTP and WebSocket) then you may simply
    // ignore the lack of close_notify.
    //
    // https://github.com/boostorg/beast/issues/38
    //
    // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
    //
    // When a short read would cut off the end of an HTTP message,
    // Beast returns the error beast::http::error::partial_message.
    // Therefore, if we see a short read here, it has occurred
    // after the message has been completed, so it is safe to ignore it.
    if (ec && ec.value() != asio::ssl::error::stream_truncated)
        Log::instance().error("client_read failed: {}", ec.what());
    co_return res;
}
