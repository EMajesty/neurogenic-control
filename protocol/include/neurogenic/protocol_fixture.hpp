#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ng::protocol_fixture {
inline constexpr std::size_t maximum_payload = 256;
inline constexpr std::size_t maximum_frame = 268;
struct Packet {
    std::uint8_t type{};
    std::uint32_t sequence{};
    std::vector<std::uint8_t> payload;
};
enum class DecodeError : std::uint8_t {
    none,
    incomplete,
    magic,
    major,
    length,
    crc
};
std::uint16_t crc16_ccitt_false(std::span<const std::uint8_t>) noexcept;
std::vector<std::uint8_t> encode(const Packet &);
std::optional<Packet> decode(std::span<const std::uint8_t>, DecodeError &) noexcept;
} // namespace ng::protocol_fixture
