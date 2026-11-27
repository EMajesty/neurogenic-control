#pragma once
#include "hardware_manifest.hpp"
#include "neurogenic/core.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace ng {
constexpr size_t kSkPixels = 58, kTlcChannels = 24, kEncodersRgb = 8;
constexpr size_t kSkBytes = kSkPixels * 3, kTlcBytes = 36;
inline constexpr auto &kEncoderTlc = hw::encoder_tlc_rgb;
struct Rgb {
    uint8_t r = 0, g = 0, b = 0;
};
enum class GammaProfile : uint8_t { linear, quadratic };
struct IlluminationStats {
    uint32_t requests = 0, completed_sk = 0, completed_tlc = 0, coalesced = 0,
             deferred = 0, errors = 0;
    uint64_t max_tlc_us = 0;
};
struct PowerPolicy {
    uint32_t limit_ua = 900000, sk_channel_ua = 20000, sk_quiescent_ua = 1000,
             tlc_channel_ua = 10500;
    uint8_t global = 128;
};
struct PowerReport {
    uint32_t requested_ua = 0, limited_ua = 0, quiescent_ua = 0;
    uint16_t scale_q12 = 4095;
};
struct ElectricalFrame {
    std::array<uint8_t, kSkBytes> sk_grb{};
    std::array<uint16_t, kTlcChannels> tlc{};
    PowerReport power{};
    uint32_t generation = 0;
};

// TLC output channel indices for each physical encoder's R,G,B terminals.
class IlluminationAuthority {
  public:
    void set_pixel(size_t, Rgb, uint8_t brightness = 255);
    void set_encoder(size_t, Rgb, uint8_t brightness = 255);
    void set_channel(size_t, uint16_t);
    void set_global(uint8_t);
    void set_gamma(GammaProfile v) { if(gamma_!=v){gamma_=v;dirty_=true;} }
    void all_off();
    bool publish(ElectricalFrame &, const PowerPolicy &);
    bool dirty() const { return dirty_; }

  private:
    std::array<Rgb, kSkPixels> pixels_{};
    std::array<Rgb, kEncodersRgb> encoders_{};
    std::array<uint8_t, kSkPixels> pixel_brightness_{};
    std::array<uint8_t, kEncodersRgb> encoder_brightness_{};
    std::array<uint16_t, kTlcChannels> direct_{};
    std::array<bool, kTlcChannels> direct_valid_{};
    uint8_t global_ = 24;
    GammaProfile gamma_ = GammaProfile::quadratic;
    bool dirty_ = true;
    uint32_t generation_ = 0;
};

void serialize_tlc(const std::array<uint16_t, kTlcChannels> &,
                   std::array<uint8_t, kTlcBytes> &);
struct TlcWire {
    virtual ~TlcWire() = default;
    virtual void data(bool) = 0;
    virtual void clock(bool) = 0;
    virtual void latch(bool) = 0;
    virtual void half_period() = 0;
};
void transmit_tlc(const std::array<uint16_t, kTlcChannels> &, TlcWire &);
class SkFrameQueue {
  public:
    void request(const std::array<uint8_t, kSkBytes> &, uint32_t generation,
                 bool priority_off = false);
    bool begin(const uint8_t *&data, size_t &size, uint32_t &generation);
    void complete();
    bool busy() const { return active_ >= 0; }
    bool pending() const { return pending_ >= 0; }
    uint32_t coalesced() const { return coalesced_; }

  private:
    std::array<std::array<uint8_t, kSkBytes>, 2> buffers_{};
    std::array<uint32_t, 2> generations_{};
    int8_t active_ = -1, pending_ = -1;
    uint32_t coalesced_ = 0;
};
} // namespace ng
