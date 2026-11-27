#pragma once
// GENERATED from hardware/illumination_calibration.json; uncalibrated defaults.
#include <array>
#include <cstdint>
namespace ng::calibration_defaults {inline constexpr uint16_t schema_version=1;inline constexpr char sha256[]="9a78aa711b8fad12dbf5fe40d8a472747f60918ab54b1218a4c0de92e2e1a57d";inline constexpr char status[]="UNCALIBRATED_CONSERVATIVE_DEFAULTS";inline constexpr std::array<uint16_t,3> sk_gain={256,256,256};inline constexpr std::array<uint16_t,3> tlc_gain={256,256,256};inline constexpr uint8_t global_ceiling=128,minimum_assigned=31;inline constexpr uint16_t transition_ms=40;inline constexpr uint8_t gamma_profile=1;inline constexpr uint32_t limit_ua=900000,sk_channel_ua=20000,sk_quiescent_ua=1000,tlc_channel_ua=10500;}
