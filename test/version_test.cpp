#include <catch2/catch_test_macros.hpp>
#include <boost/asio.hpp>
import std;
import version;

namespace net = boost::asio;

TEST_CASE("version", "[version test]") {
    net::io_context ioc;
    Version v(ioc.get_executor());
    REQUIRE(v.get_local_version().has_value());
    net::co_spawn(
            ioc,
            [&v]() -> boost::asio::awaitable<void> {
                co_await v.syncRemoteVersion();
            },
            net::detached);

    ioc.run();

    REQUIRE(v.get_remote_version().value() == v.get_local_version().value());
}
