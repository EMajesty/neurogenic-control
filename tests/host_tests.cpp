#include "harness_config.hpp"
#include "neurogenic/core.hpp"
#include "neurogenic/illumination.hpp"
#include "neurogenic/rendering.hpp"
#include <array>
#include <cassert>
#include <string>
using namespace ng;
static_assert(harness::total_pixels == 58 && harness::pixel_offset[5] == 42 &&
              harness::pixel_count[5] == 16);
struct TlcSim final : TlcWire {
    uint32_t rises = 0, latches = 0;
    bool clk = false, lat = false;
    void data(bool) override {}
    void clock(bool v) override {
        if (v && !clk)
            rises++;
        clk = v;
    }
    void latch(bool v) override {
        if (v && !lat)
            latches++;
        lat = v;
    }
    void half_period() override {}
};
struct Sim final : GpioBus {
    std::array<uint32_t, kPorts> w{};
    std::array<uint8_t, kPorts> pins{0, 1, 2, 3, 4, 5, 6, 7, 8};
    uint8_t bit = 0;
    uint64_t now = 1000;
    uint32_t reads = 0;
    bool clk = false, ld = true;
    uint32_t read_all() override {
        reads++;
        uint32_t v = 0;
        for (size_t i = 0; i < kPorts; i++)
            v |= ((w[i] >> bit) & 1u) << pins[i];
        return v;
    }
    void clock(bool v) override {
        if (v && !clk)
            bit++;
        clk = v;
    }
    void load(bool v) override {
        ld = v;
        if (!v)
            bit = 0;
    }
    void delay_us(uint32_t d) override { now += d; }
    uint64_t time_us() override { return now++; }
};
static RawSnapshot snap(uint64_t t, uint32_t bits) {
    RawSnapshot s{};
    s.timestamp_us = t;
    s.scan_index = t;
    s.port[0] = bits;
    return s;
}
int main() {
    Sim io;
    io.w = {0xa5, 0xbeef, 0xabcdef, 0, 0, 0, 0, 0, 0};
    std::array<PortConfig, kPorts> pc{};
    pc[0] = {0, 8, true};
    pc[1] = {1, 16, true};
    pc[2] = {2, 24, true};
    Scanner scanner(io, pc);
    RawSnapshot r{};
    assert(scanner.scan(r));
    assert(r.port[0] == 0xa5 && r.port[1] == 0xbeef && r.port[2] == 0xabcdef);
    assert(io.reads == 24);
    assert(!io.clk && io.ld);
    assert(scanner.acquire_led());
    assert(!scanner.scan(r));
    scanner.release_led();
    assert(scanner.scan(r));
    assert(scanner.stats().bus_contention == 1);
    std::array<ButtonConfig, kMaxInputs> bc{};
    bc[0] = {10, 0, 0, false, true};
    bc[1] = {11, 0, 1, true, true};
    Buttons buttons(bc, 5000);
    EventQueue<kEventCapacity> q;
    uint32_t seq = 0;
    auto s0 = snap(0, 1);
    buttons.baseline(s0);
    buttons.update(snap(1000, 2), q, seq);
    buttons.update(snap(5999, 2), q, seq);
    assert(q.size() == 0);
    buttons.update(snap(6000, 2), q, seq);
    assert(q.size() == 2);
    InputEvent e{};
    q.pop(e);
    assert(e.control == 10 && e.kind == EventKind::press);
    q.pop(e);
    assert(e.control == 11 && e.kind == EventKind::press);
    buttons.update(snap(20000, 1), q, seq);
    buttons.update(snap(25000, 1), q, seq);
    q.pop(e);
    assert(e.kind == EventKind::release);
    q.pop(e);
    assert(e.kind == EventKind::release);
    std::array<EncoderConfig, kMaxEncoders> ec{};
    ec[0] = {20, 0, 0, 1, 4, false, true};
    Encoders enc(ec);
    enc.baseline(snap(0, 0));
    for (auto v : {1u, 3u, 2u, 0u})
        enc.update(snap(++seq, v), q, seq);
    assert(q.pop(e));
    assert(e.kind == EventKind::encoder);
    int first = e.value;
    assert(first == 1 || first == -1);
    for (auto v : {2u, 3u, 1u, 0u})
        enc.update(snap(++seq, v), q, seq);
    assert(q.pop(e));
    assert(e.value == -first);
    enc.update(snap(++seq, 3), q, seq);
    assert(enc.stats()[0].invalid == 1);
    EventQueue<2> tiny;
    assert(tiny.push({}) && tiny.push({}) && !tiny.push({}));
    assert(tiny.overflowed());
    tiny.clear();
    assert(tiny.size() == 0);
    CommandParser p;
    Command c = Command::invalid;
    for (char x : std::string("stream on\n"))
        c = p.feed(x);
    assert(c == Command::stream_on);
    for (size_t i = 0; i < 70; i++)
        p.feed('x');
    assert(p.feed('\n') == Command::too_long);
    assert(gamma12(0) == 0 && gamma12(255) == 4095);
    LogicalLight l{255, 255, 255, 255, 255, LedState::steady};
    CurrentBudget b{20, 24, 128};
    assert(estimate_ma(&l, 1, 0, b) == 60);
    assert(budget_scale(60, b) == 85);
    IlluminationAuthority lights;
    PowerPolicy policy{};
    ElectricalFrame frame{};
    assert(lights.publish(frame, policy));
    assert(frame.power.requested_ua == 58 * 1000);
    lights.set_pixel(0, {1, 2, 3});
    lights.set_encoder(6, {255, 128, 0});
    assert(lights.publish(frame, policy));
    assert(frame.sk_grb[0] <= frame.sk_grb[1] && frame.sk_grb[3] == 0);
    assert(frame.tlc[2] > 0 && frame.tlc[1] > 0 && frame.tlc[0] == 0);
    assert(frame.power.limited_ua <= policy.limit_ua);
    std::array<uint16_t, kTlcChannels> channels{};
    channels[23] = 0x801;
    channels[0] = 1;
    std::array<uint8_t, kTlcBytes> wire{};
    serialize_tlc(channels, wire);
    assert(wire.size() == 36 && wire[0] == 0x80 && (wire[1] & 0x10) &&
           wire[35] == 1);
    TlcSim tlc_wire;
    transmit_tlc(channels, tlc_wire);
    assert(tlc_wire.rises == 288 && tlc_wire.latches == 1 && !tlc_wire.clk &&
           !tlc_wire.lat);
    SkFrameQueue frames;
    std::array<uint8_t, kSkBytes> f1{}, f2{};
    f1[0] = 1;
    f2[0] = 2;
    frames.request(f1, 1);
    frames.request(f2, 2);
    assert(frames.coalesced() == 1);
    const uint8_t *data = nullptr;
    size_t size = 0;
    uint32_t generation = 0;
    assert(frames.begin(data, size, generation) && size == 174 &&
           generation == 2 && data[0] == 2);
    assert(!frames.begin(data, size, generation));
    frames.complete();
    CommandParser lp;
    for (char x : std::string("led pixel 57 1 2 3\n"))
        c = lp.feed(x);
    assert(c == Command::led_pixel && lp.args()[0] == 57 && lp.args()[3] == 3);
    for (char x : std::string("led pixel 58 1 2 3\n"))
        c = lp.feed(x);
    assert(c == Command::invalid);
    for (char x : std::string("led channel 23 4095\n"))
        c = lp.feed(x);
    assert(c == Command::led_channel);
    assert(magnitude_brightness(MagnitudeMode::unipolar,0,31,255,true)==31);
    assert(magnitude_brightness(MagnitudeMode::unipolar,65535,31,255,true)==255);
    assert(magnitude_brightness(MagnitudeMode::bipolar,32768,31,255,true)==31);
    assert(magnitude_brightness(MagnitudeMode::bipolar,0,31,255,true)==magnitude_brightness(MagnitudeMode::bipolar,65535,31,255,true));
    assert(magnitude_brightness(MagnitudeMode::boolean,0,31,255,true)==31&&magnitude_brightness(MagnitudeMode::boolean,1,31,255,true)==255);
    assert(magnitude_brightness(MagnitudeMode::enumerated,10,31,255,true)>31);
    assert(magnitude_brightness(MagnitudeMode::unipolar,65535,31,255,false)==0);
    Calibration cal=default_calibration();assert(validate_calibration(cal));cal.sk_gain[0]=513;assert(!validate_calibration(cal));cal=default_calibration();
    Renderer renderer;IlluminationAuthority rendered;RenderState rs;rs.color=Palette::sound;rs.magnitude=0;rs.magnitude_mode=MagnitudeMode::unipolar;rs.operational=OperationalState::assigned;rs.temporal=TemporalMode::steady;rs.assigned=true;rs.explicit_off=false;renderer.set_encoder(0,rs,100);assert(renderer.render(140,rendered,cal));PowerPolicy conservative=default_power_policy();ElectricalFrame rf{};assert(rendered.publish(rf,conservative));assert(rf.tlc[kEncoderTlc[0][1]]>0&&rf.power.limited_ua<=conservative.limit_ua);
    rs.magnitude=65535;renderer.set_encoder(0,rs,110);renderer.render(130,rendered,cal);assert(rendered.publish(rf,conservative));auto halfway=rf.tlc[kEncoderTlc[0][1]];renderer.render(150,rendered,cal);assert(rendered.publish(rf,conservative));assert(rf.tlc[kEncoderTlc[0][1]]>=halfway);
    rs.magnitude_mode=MagnitudeMode::bipolar;rs.magnitude=32768;renderer.set_encoder(1,rs,UINT32_MAX-10);assert(renderer.render(20,rendered,cal));
    renderer.all_off();assert(renderer.render(21,rendered,cal));assert(rendered.publish(rf,conservative));for(auto v:rf.sk_grb)assert(v==0);for(auto v:rf.tlc)assert(v==0);
    conservative.limit_ua=0;rendered.set_pixel(0,{255,255,255});assert(rendered.publish(rf,conservative));for(auto v:rf.sk_grb)assert(v==0);assert(rf.power.limited_ua==0);
    uint16_t prior=0;for(unsigned i=0;i<256;i++){auto v=gamma12(uint8_t(i));assert(v>=prior);prior=v;}assert(prior==4095);
    for(char x:std::string("led render bipolar 7 1 2 3 -32768\n"))c=lp.feed(x);assert(c==Command::render_bipolar&&lp.args()[4]==0);for(char x:std::string("led calibration driver 1 512 256 0\n"))c=lp.feed(x);assert(c==Command::calibration_driver);
}
