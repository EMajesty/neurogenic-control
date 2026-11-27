#pragma once
#include "neurogenic/core.hpp"
#include "neurogenic/illumination.hpp"
#include <array>
#include <cstdint>
namespace ng {
class PicoIllumination {
  public:
    bool init();
    void request(const ElectricalFrame &, bool off = false);
    void service(uint64_t now_us);
    bool transfer_tlc(const ElectricalFrame &, Scanner &, uint64_t now_us);
    void enable(bool);
    bool enabled() const { return enabled_; }
    void force_blank();
    const IlluminationStats &stats() const { return stats_; }

  private:
    SkFrameQueue sk_;
    ElectricalFrame tlc_pending_{};
    bool enabled_ = false, shutdown_ = false, tlc_dirty_ = false;
    int dma_ = -1;
    unsigned pio_offset_ = 0, sm_ = 0;
    enum class Phase : uint8_t { idle, dma, reset } phase_ = Phase::idle;
    uint64_t reset_until_ = 0;
    IlluminationStats stats_{};
    std::array<uint32_t, kSkPixels> sk_words_{};
};
} // namespace ng
