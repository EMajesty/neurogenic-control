#include "neurogenic/core.hpp"
#include <stdexcept>
namespace ng {
void SharedBus::scan(size_t clocks){if(owned_)throw std::logic_error("shared bus overlap");owned_=true;bus_.event(BusEvent::acquire);bus_.event(BusEvent::load_low);bus_.event(BusEvent::load_high);for(size_t i=0;i<clocks;i++)bus_.event(BusEvent::scan_clock);bus_.event(BusEvent::release);owned_=false;}
void SharedBus::led_frame(const std::array<uint16_t,24>& c){if(owned_)throw std::logic_error("shared bus overlap");owned_=true;bus_.event(BusEvent::acquire);bus_.event(BusEvent::blank_on);for(auto v:c)for(int bit=11;bit>=0;--bit){(void)((v>>bit)&1);bus_.event(BusEvent::led_clock);}bus_.event(BusEvent::latch);bus_.event(BusEvent::load_low);bus_.event(BusEvent::load_high);bus_.event(BusEvent::blank_off);bus_.event(BusEvent::release);owned_=false;}
int Quadrature::update(bool a,bool b){static constexpr int8_t t[16]={0,-1,1,2,1,0,2,-1,-1,2,0,1,2,1,-1,0};uint8_t next=(a?2:0)|(b?1:0);int8_t d=t[(state_<<2)|next];state_=next;if(d==2){errors_++;accumulation_=0;return 0;}accumulation_+=d;if(accumulation_>=4){accumulation_=0;return 1;}if(accumulation_<=-4){accumulation_=0;return -1;}return 0;}
uint16_t gamma12(uint8_t v){uint32_t x=v;return static_cast<uint16_t>((x*x*4095u+32512u)/65025u);}
uint32_t estimate_ma(const LogicalLight* l,size_t n,uint16_t enc,const CurrentBudget&){uint64_t q=0;for(size_t i=0;i<n;i++)q+=(uint32_t(l[i].r)+l[i].g+l[i].b)*l[i].brightness*l[i].global;uint32_t sk=uint32_t((q*20u+16581375u/2)/16581375u);uint32_t tlc=(uint32_t(enc)*20u+4095u/2)/4095u;return sk+tlc;}
uint8_t budget_scale(uint32_t estimate,const CurrentBudget& b){if(!estimate||estimate<=b.limit_ma)return 255;return uint8_t((uint64_t(b.limit_ma)*255u)/estimate);}
}
