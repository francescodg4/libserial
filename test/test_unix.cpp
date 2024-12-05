#include <chrono>

#include <catch2/catch_all.hpp>
#include <fakeit.hpp>

#include "serial/serial.h"

using namespace fakeit;

TEST_CASE("serial::Timeout::simpleTimeout creates a serial::Timeout with appropriate initialization", "[serial]")
{
    auto timeout = std::chrono::milliseconds(500);

    serial::Timeout serial_timeout = serial::Timeout::simpleTimeout(static_cast<uint32_t>(timeout.count()));

    REQUIRE(serial_timeout.inter_byte_timeout == serial::Timeout::max());
    REQUIRE(serial_timeout.read_timeout_constant == static_cast<uint32_t>(timeout.count()));
    REQUIRE(serial_timeout.read_timeout_multiplier == 0);
    REQUIRE(serial_timeout.write_timeout_constant == static_cast<uint32_t>(timeout.count()));
    REQUIRE(serial_timeout.write_timeout_multiplier == 0);
}
