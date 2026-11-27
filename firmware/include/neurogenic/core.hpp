#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace ng {
enum class BusEvent:uint8_t { acquire, blank_on, load_low, load_high, scan_clock, led_clock, latch, blank_off, release };
struct SerialBus { virtual ~SerialBus()=default; virtual void event(BusEvent)=0; };
class SharedBus {
 public: explicit SharedBus(SerialBus& bus):bus_(bus){}; void scan(size_t clocks); void led_frame(const std::array<uint16_t,24>& channels);
 private: SerialBus& bus_; bool owned_=false;
};
class Quadrature {
 public: int update(bool a,bool b); uint32_t errors() const{return errors_;}
 private:uint8_t state_=0; int8_t accumulation_=0; uint32_t errors_=0;
};
enum class LedState:uint8_t { off,steady,blink,pulse,diagnostic };
struct LogicalLight {uint8_t r,g,b,brightness,global; LedState state;};
struct CurrentBudget {uint32_t limit_ma=900; uint8_t boot_global=24; uint8_t runtime_global=128;};
uint16_t gamma12(uint8_t value); uint32_t estimate_ma(const LogicalLight* lights,size_t count,uint16_t encoder_duty_sum,const CurrentBudget& budget); uint8_t budget_scale(uint32_t estimated_ma,const CurrentBudget& budget);
struct Diagnostics {uint32_t scans=0,quadrature_errors=0,bus_conflicts=0,usb_reconnects=0,current_limited_frames=0;};
}
