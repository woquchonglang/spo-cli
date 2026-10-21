module;
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
export module asio;

export namespace asio {
using boost::asio::any_completion_executor;
using boost::asio::any_io_executor;
using boost::asio::async_connect;
using boost::asio::awaitable;
using boost::asio::bad_executor;
using boost::asio::buffer;
using boost::asio::cancellation_signal;
using boost::asio::cancellation_slot;
using boost::asio::cancellation_state;
using boost::asio::cancellation_type;
using boost::asio::co_spawn;
using boost::asio::connect;
using boost::asio::coroutine;
using boost::asio::deferred;
using boost::asio::detached;
using boost::asio::detached_t;
using boost::asio::dynamic_buffer;
using boost::asio::execution_context;
using boost::asio::executor;
using boost::asio::executor_arg_t;
using boost::asio::executor_work_guard;
using boost::asio::make_work_guard;
using boost::asio::invalid_service_owner;
using boost::asio::io_context;
using boost::asio::multiple_exceptions;
using boost::asio::post;
using boost::asio::strand;
using boost::asio::service_already_exists;
using boost::asio::socket_base;
using boost::asio::static_thread_pool;
using boost::asio::steady_timer;
using boost::asio::system_context;
using boost::asio::system_executor;
using boost::asio::thread_pool;
using boost::asio::ip::address;

using boost::asio::append;
using boost::asio::as_tuple;
using boost::asio::async_compose;
using boost::asio::bind_cancellation_slot;
using boost::asio::cancel_after;
using boost::asio::consign;
using boost::asio::default_completion_token_t;
using boost::asio::detached;
using boost::asio::enable_terminal_cancellation;
using boost::asio::enable_total_cancellation;
using boost::asio::make_strand;
using boost::asio::signal_set;

namespace error {
using boost::asio::error::make_error_code;
using boost::asio::error::get_ssl_category;
using boost::asio::error::get_ssl_category;
}

namespace this_coro {

BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::this_coro::cancellation_state_t
        cancellation_state;
BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::this_coro::executor_t executor;
// using boost::asio::this_coro::cancellation_state;
// using boost::asio::this_coro::executor;
using boost::asio::this_coro::reset_cancellation_state;
using boost::asio::this_coro::throw_if_cancelled;
} // namespace this_coro

#if defined(GENERATING_DOCUMENTATION)
BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::use_awaitable_t<>
        use_awaitable;
#else
BOOST_ASIO_INLINE_VARIABLE constexpr boost::asio::use_awaitable_t<>
        use_awaitable(0, 0, 0);
#endif

namespace net {
namespace ip {
address make_address(const std::string &str) {
    return boost::asio::ip::make_address(str);
}
} // namespace ip

namespace tcp {
using boost::asio::ip::tcp::socket::shutdown_send;

using protocol = boost::asio::ip::tcp;

inline protocol v4() noexcept { return protocol::v4(); }

template <typename Protocol = protocol>
using basic_socket = boost::asio::basic_socket<Protocol>;

template <typename Protocol = protocol>
using basic_socket_acceptor = boost::asio::basic_socket_acceptor<Protocol>;

template <typename Protocol = protocol>
using basic_stream_socket = boost::asio::basic_stream_socket<Protocol>;

template <typename Protocol = protocol>
using basic_resolver = boost::asio::ip::basic_resolver<Protocol>;

using socket = basic_stream_socket<protocol>;
using acceptor = basic_socket_acceptor<protocol>;
using endpoint = boost::asio::ip::basic_endpoint<protocol>;
using resolver = basic_resolver<protocol>;
using resolver_query = boost::asio::ip::basic_resolver_query<protocol>;
using resolver_results = boost::asio::ip::basic_resolver_results<protocol>;

inline socket make_socket(io_context &ctx) { return socket(ctx); }

inline acceptor make_acceptor(io_context &ctx, const endpoint &ep) {
    acceptor a(ctx);
    a.open(ep.protocol());
    a.set_option(socket_base::reuse_address(true));
    a.bind(ep);
    a.listen();
    return a;
}

template <typename Protocol, typename... Args>
auto async_connect(basic_stream_socket<Protocol> &sock, Args &&...args) {
    return boost::asio::async_connect(sock, std::forward<Args>(args)...);
}

template <typename AsyncReadStream, typename DynamicBuffer,
          typename CompletionToken>
auto async_read_until(AsyncReadStream &stream, DynamicBuffer &&buffer,
                      const std::string &delim, CompletionToken &&token) {
    return boost::asio::async_read_until(stream,
                                         std::forward<DynamicBuffer>(buffer),
                                         delim,
                                         std::forward<CompletionToken>(token));
}

template <typename AsyncWriteStream, typename ConstBufferSequence,
          typename CompletionToken>
auto async_write(AsyncWriteStream &stream, const ConstBufferSequence &buffers,
                 CompletionToken &&token) {
    return boost::asio::async_write(stream, buffers,
                                    std::forward<CompletionToken>(token));
}

#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined no_delay;
#else
typedef boost::asio::detail::socket_option::boolean<
        BOOST_ASIO_OS_DEF(IPPROTO_TCP), BOOST_ASIO_OS_DEF(TCP_NODELAY)>
        no_delay;
#endif
} // namespace tcp

namespace udp {
using protocol = boost::asio::ip::udp;

template <typename Protocol = protocol>
using basic_socket = boost::asio::basic_socket<Protocol>;

template <typename Protocol = protocol>
using basic_endpoint = boost::asio::ip::basic_endpoint<Protocol>;

using socket = basic_socket<protocol>;
using endpoint = basic_endpoint<protocol>;
} // namespace udp
} // namespace net

namespace ssl {
using boost::asio::ssl::context;
using boost::asio::ssl::context_base;
using boost::asio::ssl::host_name_verification;
using boost::asio::ssl::stream;
using boost::asio::ssl::stream_base;
using boost::asio::ssl::verify_context;
inline constexpr int verify_peer = SSL_VERIFY_PEER;

template <typename Protocol = net::tcp::protocol>
using ssl_socket = stream<net::tcp::basic_stream_socket<Protocol>>;

template <typename Protocol>
ssl_socket<Protocol>
make_ssl_socket(net::tcp::basic_stream_socket<Protocol> &sock, context &ctx) {
    return ssl_socket<Protocol>(std::move(sock), ctx);
}

namespace error {
using boost::asio::ssl::error::stream_errors;
using boost::asio::ssl::error::stream_truncated;
} // namespace error
} // namespace ssl

namespace posix {
using boost::asio::posix::stream_descriptor;
}


} // namespace asio
