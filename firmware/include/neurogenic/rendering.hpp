#pragma once
#include "neurogenic/illumination.hpp"
#include <array>
#include <cstdint>
namespace ng {
enum class MagnitudeMode : uint8_t {
    constant,
    unipolar,
    bipolar,
    boolean,
    enumerated
};
enum class OperationalState : uint8_t {
    inactive,
    available,
    assigned,
    selected,
    active,
    muted,
    queued,
    recording,
    locked,
    unavailable
};
enum class TemporalMode : uint8_t {
    off,
    steady,
    blink,
    pulse,
    fade,
    acknowledge
};
struct RenderState {
    Rgb color{};
    uint16_t magnitude = 0;
    MagnitudeMode magnitude_mode = MagnitudeMode::constant;
    OperationalState operational = OperationalState::inactive;
    TemporalMode temporal = TemporalMode::off;
    uint16_t period_ms = 500;
    uint8_t brightness = 255;
    bool assigned = false, explicit_off = true;
};
struct Calibration {
    uint16_t schema_version = 1;
    std::array<uint16_t, 3> sk_gain{256, 256, 256}, tlc_gain{256, 256, 256};
    std::array<std::array<uint16_t, 3>, kEncodersRgb> encoder_gain{};
    std::array<std::array<uint16_t, 3>, kSkPixels> pixel_gain{};
    uint8_t global_ceiling = 128, minimum_assigned = 31;
    uint16_t transition_ms = 40;
    GammaProfile gamma = GammaProfile::quadratic;
};
struct RenderStats {
    uint32_t frames = 0, unchanged = 0, replaced_targets = 0, all_off = 0;
};
struct Palette {
    static constexpr Rgb performance{0xb5, 0x8a, 0xee},
        sequencing{0x54, 0x8c, 0xe8}, sample{0xe5, 0xa0, 0x52},
        sound{0x51, 0xbc, 0xb3}, mix{0x8a, 0xbf, 0x70},
        modulation{0xd4, 0x7b, 0xb3}, macro{0xd5, 0xca, 0x69},
        neutral{0x9b, 0xa7, 0xb8};
};
uint8_t magnitude_brightness(MagnitudeMode, uint16_t magnitude, uint8_t minimum,
                             uint8_t maximum, bool assigned);
bool validate_calibration(const Calibration &);
Calibration default_calibration();
PowerPolicy default_power_policy();
class Renderer {
  public:
    void set_pixel(size_t, const RenderState &, uint32_t now_ms);
    void set_encoder(size_t, const RenderState &, uint32_t now_ms);
    void all_off();
    void invalidate() { dirty_ = true; }
    bool render(uint32_t now_ms, IlluminationAuthority &, const Calibration &);
    const RenderStats &stats() const { return stats_; }

  private:
    struct Slot {
        RenderState state{};
        uint16_t from = 0, current = 0, target = 0;
        uint32_t changed = 0;
    };
    std::array<Slot, kSkPixels> pixels_{};
    std::array<Slot, kEncodersRgb> encoders_{};
    RenderStats stats_{};
    bool dirty_ = true, off_ = true;
    static uint8_t temporal(const Slot &, uint32_t);
    static Rgb operational(const RenderState &, uint8_t &);
    static Rgb gain(Rgb, const std::array<uint16_t, 3> &,
                    const std::array<uint16_t, 3> &);
};
} // namespace ng
