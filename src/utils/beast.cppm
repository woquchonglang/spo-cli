module;
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
export module beast;

import asio;

export namespace beast {
using boost::beast::error_code;
using boost::beast::system_error;
using boost::beast::file_mode;
using boost::beast::flat_buffer;
using boost::beast::role_type;
using boost::beast::string_view;
using boost::beast::tcp_stream;

using boost::beast::buffers_to_string;
using boost::beast::get_lowest_layer;

namespace http {
using boost::beast::http::dynamic_body;
using boost::beast::http::empty_body;
using boost::beast::http::field;
using boost::beast::http::file_body;
using boost::beast::http::request;
using boost::beast::http::response;
using boost::beast::http::status;
using boost::beast::http::string_body;
using boost::beast::http::verb;
using boost::beast::http::basic_fields;
using boost::beast::http::request_parser;

template <typename Body = string_body,
          typename Fields = boost::beast::http::fields>
auto make_request(verb method, std::string target, unsigned version = 11) {
    return request<Body, Fields>(method, target, version);
}

template <typename Stream, typename Request, typename CompletionToken>
auto async_write(Stream &stream, Request &&req, CompletionToken &&token) {
    return boost::beast::http::async_write(
            stream, std::forward<Request>(req),
            std::forward<CompletionToken>(token));
}

template <typename Stream, typename Request>
auto async_write(Stream &stream, Request &&req) {
    return boost::beast::http::async_write(stream, std::forward<Request>(req),
                                           asio::use_awaitable);
}

template <typename Stream, typename Response, typename CompletionToken>
auto async_read(Stream &stream, flat_buffer &buffer, Response &res,
                CompletionToken &&token) {
    return boost::beast::http::async_read(stream, buffer, res,
                                          std::forward<CompletionToken>(token));
}

template <typename Stream, typename Response>
auto async_read(Stream &stream, flat_buffer &buffer, Response &res) {
    return boost::beast::http::async_read(stream, buffer, res,
                                          asio::use_awaitable);
}
} // namespace http

namespace websocket {
using boost::beast::role_type;
using boost::beast::websocket::close_code;
using boost::beast::websocket::is_upgrade;
using boost::beast::websocket::request_type;
using boost::beast::websocket::response_type;
using boost::beast::websocket::stream;
using boost::beast::websocket::stream_base;

template <typename NextLayer>
auto make_websocket_stream(NextLayer &next_layer) {
    return stream<NextLayer>(next_layer);
}
} // namespace websocket

namespace ssl {
using boost::beast::ssl_stream;

template <typename Protocol>
auto make_ssl_stream(asio::net::tcp::basic_stream_socket<Protocol> &sock,
                     asio::ssl::context &ctx) {
    return ssl_stream<asio::net::tcp::basic_stream_socket<Protocol>>(
            std::move(sock), ctx);
}
} // namespace ssl
} // namespace beast
