#include "baseline_fixtures.hpp"
#include "controller_v2_wire.h"
#include "neurogenic/protocol_fixture.hpp"
#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace ng::protocol_fixture;
    static_assert(NG_CONTROLLER_PROTOCOL_MAJOR == 1);
    static_assert(NG_CONTROLLER_V2_PROTOCOL_MINOR == 3);
    static_assert(NG_CONTROLLER_V2_CONTROL_COUNT == 67);
    static_assert(NG_CONTROLLER_V2_BUTTON_COUNT == 58);
    static_assert(NG_CONTROLLER_V2_INVENTORY_FINGERPRINT == 0x3256434e);
    assert(ng::baseline_fixtures::vectors.size() == 10);
    for (const auto &fixture : ng::baseline_fixtures::vectors) {
        const Packet source{fixture.type, fixture.sequence, fixture.payload};
        assert(encode(source) == fixture.frame);
        DecodeError error{};
        const auto decoded = decode(fixture.frame, error);
        assert(decoded && error == DecodeError::none);
        assert(decoded->type == fixture.type);
        assert(decoded->sequence == fixture.sequence);
        assert(decoded->payload == fixture.payload);
    }
    auto bad_crc = ng::baseline_fixtures::vectors[2].frame;
    bad_crc.back() ^= 1;
    DecodeError error{};
    assert(!decode(bad_crc, error) && error == DecodeError::crc);
    const std::vector<std::uint8_t> oversized{0x4e, 0x47, 1, 4, 1, 1,
                                              0,    0,    0, 0, 0, 0};
    assert(!decode(oversized, error) && error == DecodeError::length);
    auto wrong_major = ng::baseline_fixtures::vectors[0].frame;
    wrong_major[2] = 2;
    assert(!decode(wrong_major, error) && error == DecodeError::major);
    assert(encode({4, 1, std::vector<std::uint8_t>(257)}).empty());
}
