#include "neurogenic/core.hpp"
#include <array>
#include <cassert>
#include <vector>
using namespace ng;
struct Sim:SerialBus{std::vector<BusEvent> e;void event(BusEvent x)override{e.push_back(x);}};
int main(){
 Quadrature q;int total=0;for(auto s:std::array<int,5>{0,1,3,2,0})total+=q.update(s&2,s&1);assert(total==-1);assert(q.errors()==0);q.update(1,1);q.update(0,0);assert(q.errors()==2);
 Sim s;SharedBus bus(s);bus.scan(24);assert(s.e.front()==BusEvent::acquire&&s.e.back()==BusEvent::release);assert(s.e[1]==BusEvent::load_low&&s.e[2]==BusEvent::load_high);
 s.e.clear();std::array<uint16_t,24> c{};bus.led_frame(c);size_t clocks=0,latches=0;for(auto x:s.e){clocks+=x==BusEvent::led_clock;latches+=x==BusEvent::latch;}assert(clocks==288&&latches==1);assert(s.e[1]==BusEvent::blank_on&&s.e[s.e.size()-2]==BusEvent::blank_off);
 assert(gamma12(0)==0&&gamma12(255)==4095);LogicalLight l{255,255,255,255,255,LedState::steady};CurrentBudget b{20,24,128};assert(estimate_ma(&l,1,0,b)==60);assert(budget_scale(60,b)==85);assert(budget_scale(0,b)==255);assert(b.boot_global==24);
}
