#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace ng {
constexpr size_t kPorts = 9, kMaxInputs = 96, kMaxEncoders = 9,
                 kEventCapacity = 64;
enum class BusOwner : uint8_t { none, scan, led };
enum class EventKind : uint8_t { press, release, encoder, overflow };
struct InputEvent {
    EventKind kind{};
    uint16_t control{};
    int16_t value{};
    uint64_t scan{};
    uint32_t sequence{};
};
template <size_t N> class EventQueue {
  public:
    bool push(InputEvent e) {
        if (size_ == N) {
            overflow_ = true;
            return false;
        }
        data_[tail_] = e;
        tail_ = (tail_ + 1) % N;
        ++size_;
        return true;
    }
    bool pop(InputEvent &e) {
        if (!size_)
            return false;
        e = data_[head_];
        head_ = (head_ + 1) % N;
        --size_;
        return true;
    }
    void clear() { head_ = tail_ = size_ = 0; }
    bool overflowed() const { return overflow_; }
    void clear_overflow() { overflow_ = false; }
    size_t size() const { return size_; }

  private:
    std::array<InputEvent, N> data_{};
    size_t head_ = 0, tail_ = 0, size_ = 0;
    bool overflow_ = false;
};
struct GpioBus {
    virtual ~GpioBus() = default;
    virtual uint32_t read_all() = 0;
    virtual void clock(bool) = 0;
    virtual void load(bool) = 0;
    virtual void delay_us(uint32_t) = 0;
    virtual uint64_t time_us() = 0;
};
struct PortConfig {
    uint8_t gpio = 0, bits = 0;
    bool enabled = false;
};
struct RawSnapshot {
    std::array<uint32_t, kPorts> port{};
    uint64_t scan_index = 0, timestamp_us = 0;
};
struct ScanStats {
    uint64_t scans = 0, last_us = 0,
             min_interval_us = std::numeric_limits<uint64_t>::max(),
             max_interval_us = 0, max_transaction_us = 0, missed_deadlines = 0,
             bus_contention = 0, event_overflow = 0;
};
class Scanner {
  public:
    Scanner(GpioBus &, std::array<PortConfig, kPorts>,
            uint32_t period_us = 250);
    bool scan(RawSnapshot &);
    bool acquire_led();
    void release_led(bool require_snapshot = true);
    const ScanStats &stats() const { return stats_; }
    void reset_stats();

  private:
    GpioBus &io_;
    std::array<PortConfig, kPorts> ports_;
    uint32_t period_us_;
    BusOwner owner_ = BusOwner::none;
    bool recovery_ = false;
    uint64_t index_ = 0;
    ScanStats stats_{};
};
struct ButtonConfig {
    uint16_t identity = 0;
    uint8_t port = 0, bit = 0;
    bool active_high = false, enabled = false;
};
struct ButtonState {
    bool raw = false, stable = false, initialized = false;
    uint64_t changed_us = 0;
};
class Buttons {
  public:
    Buttons(const std::array<ButtonConfig, kMaxInputs> &,
            uint32_t debounce_us = 5000);
    void baseline(const RawSnapshot &);
    void update(const RawSnapshot &, EventQueue<kEventCapacity> &,
                uint32_t &sequence);
    const std::array<ButtonState, kMaxInputs> &states() const {
        return states_;
    }

  private:
    bool sample(const RawSnapshot &, size_t) const;
    std::array<ButtonConfig, kMaxInputs> cfg_;
    std::array<ButtonState, kMaxInputs> states_{};
    uint32_t debounce_us_;
};
struct EncoderConfig {
    uint16_t identity = 0;
    uint8_t port = 0, bit_a = 0, bit_b = 0, counts_per_increment = 4;
    bool invert = false, enabled = false;
};
struct EncoderStats {
    uint32_t valid = 0, invalid = 0, cw = 0, ccw = 0;
    int64_t raw_position = 0, logical_position = 0;
    uint8_t ab = 0;
    bool push = false;
};
class Encoders {
  public:
    Encoders(const std::array<EncoderConfig, kMaxEncoders> &);
    void baseline(const RawSnapshot &);
    void update(const RawSnapshot &, EventQueue<kEventCapacity> &,
                uint32_t &sequence);
    void reset_stats();
    const std::array<EncoderStats, kMaxEncoders> &stats() const {
        return stats_;
    }

  private:
    std::array<EncoderConfig, kMaxEncoders> cfg_;
    std::array<EncoderStats, kMaxEncoders> stats_{};
    std::array<int16_t, kMaxEncoders> accum_{};
};
enum class Command : uint8_t {
    help,
    version,
    hardware,
    ports,
    snapshot,
    raw,
    buttons,
    encoders,
    stats,
    stream_on,
    stream_off,
    reset_stats,
    led_enable,
    led_disable,
    led_off,
    led_status,
    led_pixel,
    led_encoder,
    led_channel,
    led_brightness,
    led_walk_pixels,
    led_walk_encoders,
    led_walk_channels,
    led_test_rgb,
    led_test_white,
    led_stop,
    render_status,
    render_encoder,
    render_bipolar,
    render_unassigned,
    render_unavailable,
    render_minimum,
    render_transition,
    render_gamma_linear,
    render_gamma_quadratic,
    render_animation,
    render_stop,
    calibration_driver,
    calibration_encoder,
    calibration_pixel,
    calibration_reset,
    calibration_export,
    calibration_ramp,
    calibration_compare,
    invalid,
    too_long
};
class CommandParser {
  public:
    static constexpr size_t kMax = 63;
    Command feed(char);
    void reset();
    const std::array<uint16_t, 6> &args() const { return args_; }

  private:
    std::array<char, kMax + 1> b_{};
    std::array<uint16_t, 6> args_{};
    size_t n_ = 0;
    bool overflow_ = false;
};
enum class LedState : uint8_t { off, steady, blink, pulse, diagnostic };
struct LogicalLight {
    uint8_t r, g, b, brightness, global;
    LedState state;
};
struct CurrentBudget {
    uint32_t limit_ma = 900;
    uint8_t boot_global = 24, runtime_global = 128;
};
uint16_t gamma12(uint8_t);
uint32_t estimate_ma(const LogicalLight *, size_t, uint16_t,
                     const CurrentBudget &);
uint8_t budget_scale(uint32_t, const CurrentBudget &);
} // namespace ng
