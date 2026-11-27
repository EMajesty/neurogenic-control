#include "neurogenic/core.hpp"
#include <array>
#include <cassert>
#include <string>
using namespace ng;
struct Sim final:GpioBus{std::array<uint32_t,kPorts>w{};std::array<uint8_t,kPorts>pins{0,1,2,3,4,5,6,7,8};uint8_t bit=0;uint64_t now=1000;uint32_t reads=0;bool clk=false,ld=true;uint32_t read_all()override{reads++;uint32_t v=0;for(size_t i=0;i<kPorts;i++)v|=((w[i]>>bit)&1u)<<pins[i];return v;}void clock(bool v)override{if(v&&!clk)bit++;clk=v;}void load(bool v)override{ld=v;if(!v)bit=0;}void delay_us(uint32_t d)override{now+=d;}uint64_t time_us()override{return now++;}};
static RawSnapshot snap(uint64_t t,uint32_t bits){RawSnapshot s{};s.timestamp_us=t;s.scan_index=t;s.port[0]=bits;return s;}
int main(){
 Sim io;io.w={0xa5,0xbeef,0xabcdef,0,0,0,0,0,0};std::array<PortConfig,kPorts> pc{};pc[0]={0,8,true};pc[1]={1,16,true};pc[2]={2,24,true};Scanner scanner(io,pc);RawSnapshot r{};assert(scanner.scan(r));assert(r.port[0]==0xa5&&r.port[1]==0xbeef&&r.port[2]==0xabcdef);assert(io.reads==24);assert(!io.clk&&io.ld);assert(scanner.acquire_led());assert(!scanner.scan(r));scanner.release_led();assert(scanner.scan(r));assert(scanner.stats().bus_contention==1);
 std::array<ButtonConfig,kMaxInputs> bc{};bc[0]={10,0,0,false,true};bc[1]={11,0,1,true,true};Buttons buttons(bc,5000);EventQueue<kEventCapacity> q;uint32_t seq=0;auto s0=snap(0,1);buttons.baseline(s0);buttons.update(snap(1000,2),q,seq);buttons.update(snap(5999,2),q,seq);assert(q.size()==0);buttons.update(snap(6000,2),q,seq);assert(q.size()==2);InputEvent e{};q.pop(e);assert(e.control==10&&e.kind==EventKind::press);q.pop(e);assert(e.control==11&&e.kind==EventKind::press);buttons.update(snap(20000,1),q,seq);buttons.update(snap(25000,1),q,seq);q.pop(e);assert(e.kind==EventKind::release);q.pop(e);assert(e.kind==EventKind::release);
 std::array<EncoderConfig,kMaxEncoders> ec{};ec[0]={20,0,0,1,4,false,true};Encoders enc(ec);enc.baseline(snap(0,0));for(auto v:{1u,3u,2u,0u})enc.update(snap(++seq,v),q,seq);assert(q.pop(e));assert(e.kind==EventKind::encoder);int first=e.value;assert(first==1||first==-1);for(auto v:{2u,3u,1u,0u})enc.update(snap(++seq,v),q,seq);assert(q.pop(e));assert(e.value==-first);enc.update(snap(++seq,3),q,seq);assert(enc.stats()[0].invalid==1);
 EventQueue<2> tiny;assert(tiny.push({})&&tiny.push({})&&!tiny.push({}));assert(tiny.overflowed());tiny.clear();assert(tiny.size()==0);
 CommandParser p;Command c=Command::invalid;for(char x:std::string("stream on\n"))c=p.feed(x);assert(c==Command::stream_on);for(size_t i=0;i<70;i++)p.feed('x');assert(p.feed('\n')==Command::too_long);
 assert(gamma12(0)==0&&gamma12(255)==4095);LogicalLight l{255,255,255,255,255,LedState::steady};CurrentBudget b{20,24,128};assert(estimate_ma(&l,1,0,b)==60);assert(budget_scale(60,b)==85);
}
