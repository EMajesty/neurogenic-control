#include "neurogenic/protocol_fixture.hpp"
#include "controller_v2_wire.h"

namespace ng::protocol_fixture {
namespace {
constexpr std::uint16_t magic = 0x474e;
std::uint16_t read16(const std::uint8_t *p) {
    return std::uint16_t(p[0]) | std::uint16_t(p[1]) << 8;
}
std::uint32_t read32(const std::uint8_t *p) {
    return std::uint32_t(p[0]) | std::uint32_t(p[1]) << 8 |
           std::uint32_t(p[2]) << 16 | std::uint32_t(p[3]) << 24;
}
void append16(std::vector<std::uint8_t> &v, std::uint16_t n) {
    v.push_back(n & 255);
    v.push_back(n >> 8);
}
void append32(std::vector<std::uint8_t> &v, std::uint32_t n) {
    for (int i = 0; i < 4; ++i)
        v.push_back((n >> (i * 8)) & 255);
}
} // namespace

std::uint16_t crc16_ccitt_false(std::span<const std::uint8_t> data) noexcept {
    std::uint16_t value = 0xffff;
    for (auto byte : data) {
        value ^= std::uint16_t(byte) << 8;
        for (int i = 0; i < 8; ++i)
            value = value & 0x8000 ? std::uint16_t((value << 1) ^ 0x1021)
                                   : std::uint16_t(value << 1);
    }
    return value;
}

std::vector<std::uint8_t> encode(const Packet &packet) {
    if (packet.payload.size() > maximum_payload)
        return {};
    std::vector<std::uint8_t> out;
    out.reserve(packet.payload.size() + 12);
    append16(out, magic);
    out.push_back(NG_CONTROLLER_PROTOCOL_MAJOR);
    out.push_back(packet.type);
    append16(out, static_cast<std::uint16_t>(packet.payload.size()));
    append32(out, packet.sequence);
    out.insert(out.end(), packet.payload.begin(), packet.payload.end());
    append16(out, crc16_ccitt_false(out));
    return out;
}

std::optional<Packet> decode(std::span<const std::uint8_t> bytes,
                             DecodeError &error) noexcept {
    if (bytes.size() < 12) {
        error = DecodeError::incomplete;
        return {};
    }
    if (read16(bytes.data()) != magic) {
        error = DecodeError::magic;
        return {};
    }
    if (bytes[2] != NG_CONTROLLER_PROTOCOL_MAJOR) {
        error = DecodeError::major;
        return {};
    }
    const auto length = read16(bytes.data() + 4);
    if (length > maximum_payload) {
        error = DecodeError::length;
        return {};
    }
    const auto total = std::size_t(length) + 12;
    if (bytes.size() < total) {
        error = DecodeError::incomplete;
        return {};
    }
    if (crc16_ccitt_false(bytes.first(total - 2)) !=
        read16(bytes.data() + total - 2)) {
        error = DecodeError::crc;
        return {};
    }
    error = DecodeError::none;
    Packet result{bytes[3], read32(bytes.data() + 6), {}};
    result.payload.assign(bytes.begin() + 10, bytes.begin() + 10 + length);
    return result;
}
} // namespace ng::protocol_fixture
