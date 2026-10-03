#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <boost/asio.hpp>

import std;
import spotifyWebAPI;
import spotifyAuthPKCE;
import config;

TEST_CASE("auth PKCE", "[auth PKCE test]") {
    boost::asio::io_context ioc;
    Config config;
    SpotifyAuthPKCE auth(ioc, config);
    bool islogin = false;
    boost::asio::co_spawn(
            ioc,
            [&ioc, &auth, &islogin]() -> boost::asio::awaitable<void> {
                islogin = co_await auth.login();
                ioc.stop();
            },
            boost::asio::detached);
    ioc.run();

    REQUIRE(islogin == true);
}
