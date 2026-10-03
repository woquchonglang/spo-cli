module;
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
export module http.client;

import std;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

export net::awaitable<http::response<http::dynamic_body>>
client_read(std::string host, std::string port, ssl::context &ctx,
            http::request<http::string_body> req) {
    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{ executor };
    auto stream = ssl::stream<beast::tcp_stream>{ executor, ctx };

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        throw beast::system_error(static_cast<int>(::ERR_get_error()),
                                  net::error::get_ssl_category());
    }

    // Set the expected hostname in the peer certificate for verification
    stream.set_verify_callback(ssl::host_name_verification(host));

    // Look up the domain name
    auto const results =
            co_await resolver.async_resolve(host, port, net::use_awaitable);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));

    // Make the connection on the IP address we get from a lookup
    co_await beast::get_lowest_layer(stream).async_connect(results,
                                                           net::use_awaitable);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));

    // Perform the SSL handshake
    co_await stream.async_handshake(ssl::stream_base::client,
                                    net::use_awaitable);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(15));

    // Send the HTTP request to the remote host
    co_await http::async_write(stream, req, net::use_awaitable);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    co_await http::async_read(stream, buffer, res, net::use_awaitable);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(1));

    // Gracefully close the stream - do not threat every error as an exception!
    auto [ec] = co_await stream.async_shutdown(net::as_tuple);

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
    if (ec && ec != net::ssl::error::stream_truncated)
        throw boost::system::system_error(ec, "shutdown");

    co_return res;
}
