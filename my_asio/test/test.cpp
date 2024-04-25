#include <catch2/catch_test_macros.hpp>

#include "asio.hpp"

TEST_CASE("test", "test")
{
	REQUIRE(my_asio::test());
}

