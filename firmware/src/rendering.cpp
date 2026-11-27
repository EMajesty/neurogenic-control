#include "neurogenic/rendering.hpp"
#include "illumination_calibration.hpp"
#include <algorithm>
namespace ng {
uint8_t magnitude_brightness(MagnitudeMode m, uint16_t v, uint8_t lo,
                             uint8_t hi, bool assigned) {
    if (!assigned)
        return 0;
    v = std::min<uint16_t>(v, 65535);
    uint32_t n = 65535;
    if (m == MagnitudeMode::unipolar)
        n = v;
    else if (m == MagnitudeMode::bipolar) {
        int32_t s = int32_t(v) - 32768;
        n = uint32_t(s < 0 ? -s : s) * 2u;
        n = std::min<uint32_t>(n, 65535);
    } else if (m == MagnitudeMode::boolean)
        n = v ? 65535 : 0;
    else if (m == MagnitudeMode::enumerated)
        n = 32768;
    return uint8_t(lo + (uint32_t(hi - lo) * n + 32767) / 65535);
}
Calibration default_calibration() {
    Calibration c;
    c.sk_gain = calibration_defaults::sk_gain;
    c.tlc_gain = calibration_defaults::tlc_gain;
    for (auto &x : c.encoder_gain)
        x = {256, 256, 256};
    for (auto &x : c.pixel_gain)
        x = {256, 256, 256};
    c.global_ceiling = calibration_defaults::global_ceiling;
    c.minimum_assigned = calibration_defaults::minimum_assigned;
    c.transition_ms = calibration_defaults::transition_ms;
    c.gamma = GammaProfile(calibration_defaults::gamma_profile);
    return c;
}
PowerPolicy default_power_policy() {
    return {calibration_defaults::limit_ua, calibration_defaults::sk_channel_ua,
            calibration_defaults::sk_quiescent_ua,
            calibration_defaults::tlc_channel_ua,
            calibration_defaults::global_ceiling};
}
bool validate_calibration(const Calibration &c) {
    if (c.schema_version != 1 || c.global_ceiling > 192 ||
        c.minimum_assigned > 128 || c.transition_ms > 1000)
        return false;
    auto good = [](auto &a) {
        for (auto v : a)
            if (v > 512)
                return false;
        return true;
    };
    if (!good(c.sk_gain) || !good(c.tlc_gain))
        return false;
    for (auto &a : c.encoder_gain)
        if (!good(a))
            return false;
    for (auto &a : c.pixel_gain)
        if (!good(a))
            return false;
    return c.gamma == GammaProfile::linear ||
           c.gamma == GammaProfile::quadratic;
}
static uint16_t target_for(const RenderState &s, uint8_t minimum) {
    if (s.explicit_off || s.temporal == TemporalMode::off ||
        s.operational == OperationalState::unavailable)
        return 0;
    return magnitude_brightness(s.magnitude_mode, s.magnitude, minimum,
                                s.brightness, s.assigned);
}
void Renderer::set_pixel(size_t i, const RenderState &s, uint32_t now) {
    if (i >= pixels_.size())
        return;
    auto &x = pixels_[i];
    x.from = x.current;
    x.state = s;
    x.target = target_for(s, 0);
    x.changed = now;
    dirty_ = true;
    off_ = false;
    stats_.replaced_targets++;
}
void Renderer::set_encoder(size_t i, const RenderState &s, uint32_t now) {
    if (i >= encoders_.size())
        return;
    auto &x = encoders_[i];
    x.from = x.current;
    x.state = s;
    x.target = target_for(s, 31);
    x.changed = now;
    dirty_ = true;
    off_ = false;
    stats_.replaced_targets++;
}
void Renderer::all_off() {
    pixels_ = {};
    encoders_ = {};
    dirty_ = true;
    off_ = true;
    stats_.all_off++;
}
uint8_t Renderer::temporal(const Slot &s, uint32_t now) {
    if (!s.target)
        return 0;
    uint32_t elapsed = now - s.changed,
             period = std::max<uint16_t>(s.state.period_ms, 40);
    switch (s.state.temporal) {
    case TemporalMode::blink:
        return (elapsed % period) < period / 2 ? 255 : 0;
    case TemporalMode::pulse: {
        uint32_t p = elapsed % period, half = period / 2;
        return uint8_t(64 + (191u * (p < half ? p : period - p)) /
                                std::max<uint32_t>(half, 1));
    }
    case TemporalMode::fade:
        return elapsed >= period ? 0
                                 : uint8_t(255u * (period - elapsed) / period);
    case TemporalMode::acknowledge:
        return elapsed < period ? 255 : 0;
    case TemporalMode::off:
        return 0;
    default:
        return 255;
    }
}
Rgb Renderer::operational(const RenderState &s, uint8_t &scale) {
    scale = 255;
    switch (s.operational) {
    case OperationalState::recording:
        return {255, 32, 24};
    case OperationalState::queued:
        return {255, 150, 32};
    case OperationalState::selected:
        return {255, 255, 255};
    case OperationalState::active:
        return {80, 210, 96};
    case OperationalState::locked:
        return {181, 138, 238};
    case OperationalState::muted:
        scale = 72;
        break;
    case OperationalState::unavailable:
        return {};
    default:
        break;
    }
    return s.color;
}
Rgb Renderer::gain(Rgb v, const std::array<uint16_t, 3> &a,
                   const std::array<uint16_t, 3> &b) {
    auto g = [](uint8_t x, uint16_t y, uint16_t z) {
        return uint8_t(
            std::min<uint32_t>(255, (uint32_t(x) * y * z + 32768) / 65536));
    };
    return {g(v.r, a[0], b[0]), g(v.g, a[1], b[1]), g(v.b, a[2], b[2])};
}
bool Renderer::render(uint32_t now, IlluminationAuthority &out,
                      const Calibration &c) {
    if (!validate_calibration(c))
        return false;
    out.set_gamma(c.gamma);
    if (off_) {
        if(!dirty_){stats_.unchanged++;return false;}
        out.all_off();
        dirty_ = false;
        stats_.frames++;
        return true;
    }
    bool animated = false,needs=dirty_;
    auto check=[&](const auto&slots){for(const auto&s:slots){uint32_t elapsed=now-s.changed;if((c.transition_ms&&elapsed<c.transition_ms&&s.target)||(s.state.temporal!=TemporalMode::steady&&s.state.temporal!=TemporalMode::off))needs=true;}};check(pixels_);check(encoders_);if(!needs){stats_.unchanged++;return false;}
    auto draw = [&](auto &slots, bool enc) {
        for (size_t i = 0; i < slots.size(); i++) {
            auto &s = slots[i];
            uint16_t desired=target_for(s.state,enc?c.minimum_assigned:0);if(desired!=s.target){s.from=s.current;s.target=desired;s.changed=now;}
            uint32_t elapsed = now - s.changed;
            if (c.transition_ms && elapsed < c.transition_ms && s.target) {
                s.current =
                    uint16_t(s.from + (int32_t(s.target) - s.from) *
                                          int32_t(elapsed) / c.transition_ms);
                animated = true;
            } else
                s.current = s.target;
            uint8_t op = 255;
            Rgb color = operational(s.state, op);
            uint8_t t = temporal(s, now);
            if (s.state.temporal != TemporalMode::steady &&
                s.state.temporal != TemporalMode::off)
                animated = true;
            uint8_t b = uint8_t(uint32_t(s.current) * op * t / 65025u);
            if (enc) {
                color = gain(color, c.tlc_gain, c.encoder_gain[i]);
                out.set_encoder(i, color, b);
            } else {
                color = gain(color, c.sk_gain, c.pixel_gain[i]);
                out.set_pixel(i, color, b);
            }
        }
    };
    draw(pixels_, false);
    draw(encoders_, true);
    out.set_global(c.global_ceiling);
    dirty_ = animated;
    stats_.frames++;
    return true;
}
} // namespace ng
