#include "hardware_manifest.hpp"
#include "harness_config.hpp"
#include "illumination_calibration.hpp"
#include "neurogenic/core.hpp"
#include "neurogenic/illumination.hpp"
#include "neurogenic/rendering.hpp"
#include "pico/stdlib.h"
#include "pico_gpio.hpp"
#include "pico_illumination.hpp"
#include <array>
#include <cstdio>
#include <cstring>
using namespace ng;
enum class Animation : uint8_t { none, pixels, encoders, channels, rgb };
static Animation animation = Animation::none;
static size_t animation_index = 0;
static uint64_t animation_next = 0;
static uint32_t diagnostic_output_overflow = 0, input_event_overflow = 0;
static std::array<std::array<uint32_t, 24>, kPorts> raw_transitions{};
static constexpr uint64_t kIlluminationSessionUs = 30000000;
struct StressState {
    bool running = false;
    uint64_t started_us = 0, ends_us = 0, baseline_missed = 0,
             baseline_contention = 0;
    uint32_t baseline_invalid = 0, baseline_errors = 0;
};

static std::array<ButtonConfig, kMaxInputs> button_config() {
    std::array<ButtonConfig, kMaxInputs> out{};
    size_t n = 0;
    for (uint8_t p = 0; p < kPorts; p++) {
        uint32_t mask = 0;
        bool high = false;
        switch (harness::module_type[p]) {
        case 1:
            mask = 0xff;
            break;
        case 2:
            mask = 0xf8f8;
            break;
        case 3:
            mask = 0xcf;
            break;
        case 4:
            mask = 0xffff;
            break;
        case 5:
            mask = 0xff;
            high = true;
            break;
        default:
            break;
        }
        for (uint8_t bit = 0; bit < 24 && n < out.size(); bit++)
            if (mask & (1u << bit))
                out[n++] = {uint16_t(p * 32 + bit), p, bit, high, true};
    }
    return out;
}
static std::array<EncoderConfig, kMaxEncoders> encoder_config() {
    std::array<EncoderConfig, kMaxEncoders> out{};
    const uint8_t a[8] = {21, 19, 22, 16, 13, 11, 14, 8},
                  b[8] = {20, 18, 23, 17, 12, 10, 15, 9};
    for (uint8_t i = 0; i < 8; i++)
        out[i] = {uint16_t(i), 8, a[i], b[i], 4, false, true};
    for (uint8_t p = 0; p < 8; p++)
        if (harness::module_type[p] == 3)
            out[8] = {8, p, 5, 4, 4, false, true};
    return out;
}
static void print_command(Command c, const std::array<uint16_t, 6> &a,
                          const RawSnapshot &s, Scanner &scan, Buttons &buttons,
                          Encoders &enc, bool &stream,
                          IlluminationAuthority &lights,
                          PicoIllumination &driver,
                          const ElectricalFrame &frame, Renderer &renderer,
                          Calibration &calibration, PowerPolicy &power,
                          StressState &stress, uint64_t now) {
    switch (c) {
    case Command::help:
        std::puts(
            "OK help info inventory module version hardware ports snapshot raw "
            "buttons "
            "encoders stats stream on|off reset-stats heartbeat stress; led "
            "enable|disable|off|status|pixel|encoder|channel|brightness|"
            "walk|test|stop");
        break;
    case Command::info:
        std::printf("NGD1 type=identity diag=%u firmware=%s protocol=1.3 "
                    "hardware=%s source=%s dirty=%u manifest_schema=%u "
                    "manifest=%s harness=%s calibration=%s sdk=%s\r\n",
                    NG_DIAGNOSTIC_VERSION, NG_FIRMWARE_VERSION,
                    hw::hardware_baseline, NG_SOURCE_GIT_SHA, NG_SOURCE_DIRTY,
                    hw::manifest_schema_version, hw::manifest_sha256,
                    harness::sha256, calibration_defaults::sha256,
                    NG_PICO_SDK_VERSION);
        std::puts(
            "NGD1 type=capabilities "
            "values=raw,inventory,events,encoders,illumination,calibration,"
            "timing,stress,reports watchdog_s=30 max_command=63");
        break;
    case Command::inventory:
        std::printf("NGD1 type=inventory physical_buttons=58 encoders=9 "
                    "optional_pushes=8 wire_controls=67 manifest=%s\r\n",
                    hw::manifest_sha256);
        for (size_t i = 0; i < kPorts; ++i)
            std::printf("NGD1 type=port index=%u name=%s%u module_type=%u "
                        "chain_bits=%u enabled=%u presence=unknown raw=0x%06lx "
                        "scan=%llu\r\n",
                        static_cast<unsigned>(i), i == 8 ? "ENC" : "MOD",
                        static_cast<unsigned>(i == 8 ? 0 : i + 1),
                        harness::module_type[i], harness::chain_bits[i],
                        harness::enabled[i],
                        static_cast<unsigned long>(s.port[i]), s.scan_index);
        for (size_t i = 0; i < kPorts; ++i) {
            auto type = harness::module_type[i];
            for (size_t bit = 0; bit < harness::chain_bits[i]; ++bit) {
                const char *name = hw::module_bit_name[type][bit];
                const char *role =
                    !std::strcmp(name, "NC") ? "unused"
                    : (std::strstr(name, "_A") || std::strstr(name, "_B") ||
                       !std::strcmp(name, "DATA.A") ||
                       !std::strcmp(name, "DATA.B"))
                        ? "quadrature"
                        : "button";
                const char *polarity = !std::strcmp(role, "quadrature")
                                           ? "level"
                                       : (type == 5 && bit < 8) ? "active-high"
                                                                : "active-low";
                std::printf(
                    "NGD1 type=input port=%u bit=%u physical=%s role=%s "
                    "polarity=%s value=%u transitions=%lu\r\n",
                    static_cast<unsigned>(i), static_cast<unsigned>(bit), name,
                    role, polarity,
                    static_cast<unsigned>((s.port[i] >> bit) & 1u),
                    static_cast<unsigned long>(raw_transitions[i][bit]));
            }
        }
        break;
    case Command::module: {
        auto i = a[0];
        uint64_t transitions = 0;
        for (size_t bit = 0; bit < harness::chain_bits[i]; ++bit)
            transitions += raw_transitions[i][bit];
        std::printf("NGD1 type=module index=%u name=%s%u module_type=%u "
                    "chain_bits=%u enabled=%u presence=unknown raw=0x%06lx "
                    "transitions=%llu scan=%llu timestamp_us=%llu\r\n",
                    i, i == 8 ? "ENC" : "MOD", i == 8 ? 0 : i + 1,
                    harness::module_type[i], harness::chain_bits[i],
                    harness::enabled[i], static_cast<unsigned long>(s.port[i]),
                    transitions, s.scan_index, s.timestamp_us);
        break;
    }
    case Command::version:
        std::printf("NEUROGENIC CONTROL %s MODE DIAGNOSTIC PROTOCOL 1.3\r\n",
                    NG_FIRMWARE_VERSION);
        break;
    case Command::hardware:
        std::printf("HARDWARE %s SCAN_TARGET_HZ 4000 SK6812=58 TLC5947=24 "
                    "ILLUMINATION default-disabled\r\n",
                    hw::hardware_baseline);
        break;
    case Command::ports:
        std::printf("PORTS status=%s presence=unknown\r\n", harness::status);
        break;
    case Command::snapshot:
    case Command::raw:
        for (size_t i = 0; i < 8; i++)
            std::printf("PORT MOD%u RAW 0x%06lx\r\n",
                        static_cast<unsigned>(i + 1),
                        static_cast<unsigned long>(s.port[i]));
        std::printf("PORT ENC RAW 0x%06lx\r\n",
                    static_cast<unsigned long>(s.port[8]));
        break;
    case Command::buttons: {
        size_t i = 0;
        for (auto &x : buttons.states())
            if (x.initialized && x.stable)
                i++;
        std::printf("BUTTONS pressed=%u authoritative_scan=%llu\r\n",
                    static_cast<unsigned>(i), s.scan_index);
        break;
    }
    case Command::encoders:
        for (size_t i = 0; i < kMaxEncoders; i++) {
            auto &x = enc.stats()[i];
            std::printf("ENCODER %s%u ab=%u valid=%lu invalid=%lu cw=%lu "
                        "ccw=%lu raw=%lld logical=%lld\r\n",
                        i == 8 ? "DATA" : "P",
                        static_cast<unsigned>(i == 8 ? 1 : i + 1), x.ab,
                        static_cast<unsigned long>(x.valid),
                        static_cast<unsigned long>(x.invalid),
                        static_cast<unsigned long>(x.cw),
                        static_cast<unsigned long>(x.ccw), x.raw_position,
                        x.logical_position);
        }
        break;
    case Command::stats: {
        auto &x = scan.stats();
        auto &ls = driver.stats();
        uint64_t intervals = x.scans > 1 ? x.scans - 1 : 0;
        std::printf(
            "NGD1 type=stats scan_target_hz=4000 scans=%llu missed=%llu "
            "contention=%llu min_us=%llu "
            "max_us=%llu mean_us=%llu input_max_us=%llu tlc_max_us=%llu "
            "led_requests=%lu sk_complete=%lu tlc_complete=%lu coalesced=%lu "
            "deferred=%lu driver_errors=%lu event_overflow=%lu "
            "output_overflow=%lu\r\n",
            x.scans, x.missed_deadlines, x.bus_contention,
            x.min_interval_us == UINT64_MAX ? 0 : x.min_interval_us,
            x.max_interval_us, intervals ? x.interval_total_us / intervals : 0,
            x.max_transaction_us, ls.max_tlc_us,
            static_cast<unsigned long>(ls.requests),
            static_cast<unsigned long>(ls.completed_sk),
            static_cast<unsigned long>(ls.completed_tlc),
            static_cast<unsigned long>(ls.coalesced),
            static_cast<unsigned long>(ls.deferred),
            static_cast<unsigned long>(ls.errors),
            static_cast<unsigned long>(input_event_overflow),
            static_cast<unsigned long>(diagnostic_output_overflow));
        break;
    }
    case Command::stream_on:
        stream = true;
        std::puts("OK stream on rate_hz=10");
        break;
    case Command::stream_off:
        stream = false;
        std::puts("OK stream off");
        break;
    case Command::reset_stats:
        scan.reset_stats();
        enc.reset_stats();
        diagnostic_output_overflow = input_event_overflow = 0;
        raw_transitions = {};
        std::puts("OK stats reset");
        break;
    case Command::heartbeat:
        std::puts("NGD1 type=heartbeat status=ok");
        break;
    case Command::stress_start: {
        uint32_t invalid = 0;
        for (const auto &e : enc.stats())
            invalid += e.invalid;
        stress = {true,
                  now,
                  now + uint64_t(a[0]) * 1000000,
                  scan.stats().missed_deadlines,
                  scan.stats().bus_contention,
                  invalid,
                  driver.stats().errors};
        animation = Animation::rgb;
        std::printf("NGD1 type=stress state=running duration_s=%u "
                    "pattern=conservative-current-limited\r\n",
                    a[0]);
        break;
    }
    case Command::stress_stop:
        stress.running = false;
        animation = Animation::none;
        lights.all_off();
        std::puts("NGD1 type=stress state=stopped all_off=queued");
        break;
    case Command::stress_status: {
        uint32_t invalid = 0;
        for (const auto &e : enc.stats())
            invalid += e.invalid;
        std::printf(
            "NGD1 type=stress state=%s elapsed_ms=%llu missed_delta=%llu "
            "contention_delta=%llu invalid_delta=%lu errors_delta=%lu\r\n",
            stress.running ? "running" : "stopped",
            stress.started_us ? (now - stress.started_us) / 1000 : 0,
            scan.stats().missed_deadlines - stress.baseline_missed,
            scan.stats().bus_contention - stress.baseline_contention,
            static_cast<unsigned long>(invalid - stress.baseline_invalid),
            static_cast<unsigned long>(driver.stats().errors -
                                       stress.baseline_errors));
        break;
    }
    case Command::led_enable:
        driver.enable(true);
        std::puts("OK led enabled external-5v-confirmed-by-operator");
        break;
    case Command::led_disable:
        animation = Animation::none;
        lights.all_off();
        driver.enable(false);
        std::puts("OK led disabled blank-asserted clear-frame-queued");
        break;
    case Command::led_off:
    case Command::led_stop:
        animation = Animation::none;
        lights.all_off();
        std::puts("OK led off queued");
        break;
    case Command::led_status: {
        auto &x = driver.stats();
        std::printf(
            "LED enabled=%u requested_ua=%lu limited_ua=%lu limit_ua=900000 "
            "scale_q12=%u requests=%lu sk_complete=%lu tlc_complete=%lu "
            "coalesced=%lu deferred=%lu tlc_max_us=%llu\r\n",
            driver.enabled(), (unsigned long)frame.power.requested_ua,
            (unsigned long)frame.power.limited_ua, frame.power.scale_q12,
            (unsigned long)x.requests, (unsigned long)x.completed_sk,
            (unsigned long)x.completed_tlc, (unsigned long)x.coalesced,
            (unsigned long)x.deferred, x.max_tlc_us);
        break;
    }
    case Command::led_pixel:
        lights.set_pixel(a[0], {uint8_t(a[1]), uint8_t(a[2]), uint8_t(a[3])});
        std::printf("OK led physical-pixel=%u\r\n", a[0]);
        break;
    case Command::led_encoder:
        lights.set_encoder(a[0], {uint8_t(a[1]), uint8_t(a[2]), uint8_t(a[3])});
        std::printf("OK led physical-encoder=%u\r\n", a[0]);
        break;
    case Command::led_channel:
        lights.set_channel(a[0], a[1]);
        std::printf("OK led tlc-out=%u value=%u\r\n", a[0], a[1]);
        break;
    case Command::led_brightness:
        lights.set_global(uint8_t(a[0]));
        std::printf("OK led brightness=%u capped-by-policy\r\n", a[0]);
        break;
    case Command::led_walk_pixels:
        animation = Animation::pixels;
        animation_index = 0;
        animation_next = 0;
        std::puts("OK led walk physical-pixels");
        break;
    case Command::led_walk_encoders:
        animation = Animation::encoders;
        animation_index = 0;
        animation_next = 0;
        std::puts("OK led walk physical-encoders");
        break;
    case Command::led_walk_channels:
        animation = Animation::channels;
        animation_index = 0;
        animation_next = 0;
        std::puts("OK led walk tlc-channels");
        break;
    case Command::led_test_rgb:
        animation = Animation::rgb;
        animation_index = 0;
        animation_next = 0;
        std::puts("OK led rgb test");
        break;
    case Command::led_test_white:
        animation = Animation::none;
        for (size_t i = 0; i < 58; i++)
            lights.set_pixel(i, {255, 255, 255});
        for (size_t i = 0; i < 8; i++)
            lights.set_encoder(i, {255, 255, 255});
        std::puts("OK led white requested current-limited");
        break;
    case Command::render_status:
        std::printf("RENDER minimum=%u transition_ms=%u gamma=%s ceiling=%u "
                    "model_sk_channel_ua=%lu model_sk_quiescent_ua=%lu "
                    "frames=%lu unchanged=%lu replaced=%lu\r\n",
                    calibration.minimum_assigned, calibration.transition_ms,
                    calibration.gamma == GammaProfile::quadratic ? "quadratic"
                                                                 : "linear",
                    calibration.global_ceiling,
                    (unsigned long)power.sk_channel_ua,
                    (unsigned long)power.sk_quiescent_ua,
                    (unsigned long)renderer.stats().frames,
                    (unsigned long)renderer.stats().unchanged,
                    (unsigned long)renderer.stats().replaced_targets);
        break;
    case Command::render_encoder:
    case Command::render_bipolar: {
        RenderState r;
        r.color = {uint8_t(a[1]), uint8_t(a[2]), uint8_t(a[3])};
        r.magnitude = a[4];
        r.magnitude_mode = c == Command::render_bipolar
                               ? MagnitudeMode::bipolar
                               : MagnitudeMode::unipolar;
        r.operational = OperationalState::assigned;
        r.temporal = TemporalMode::steady;
        r.assigned = true;
        r.explicit_off = false;
        renderer.set_encoder(a[0], r, uint32_t(time_us_64() / 1000));
        std::puts("OK render encoder target replaced");
        break;
    }
    case Command::render_unassigned: {
        RenderState r;
        r.explicit_off = true;
        renderer.set_encoder(a[0], r, uint32_t(time_us_64() / 1000));
        std::puts("OK render unassigned off");
        break;
    }
    case Command::render_unavailable: {
        RenderState r;
        r.operational = OperationalState::unavailable;
        r.temporal = TemporalMode::steady;
        r.explicit_off = false;
        renderer.set_encoder(a[0], r, uint32_t(time_us_64() / 1000));
        std::puts("OK render unavailable off");
        break;
    }
    case Command::render_minimum:
        calibration.minimum_assigned = uint8_t(a[0]);
        renderer.invalidate();
        std::puts("OK render minimum provisional");
        break;
    case Command::render_transition:
        calibration.transition_ms = a[0];
        renderer.invalidate();
        std::puts("OK render transition");
        break;
    case Command::render_gamma_linear:
        calibration.gamma = GammaProfile::linear;
        renderer.invalidate();
        std::puts("OK render gamma linear");
        break;
    case Command::render_gamma_quadratic:
        calibration.gamma = GammaProfile::quadratic;
        renderer.invalidate();
        std::puts("OK render gamma quadratic");
        break;
    case Command::render_animation: {
        RenderState r;
        r.color = Palette::neutral;
        r.magnitude = 65535;
        r.magnitude_mode = MagnitudeMode::constant;
        r.operational = OperationalState::assigned;
        r.temporal = TemporalMode(a[1]);
        r.assigned = true;
        r.explicit_off = a[1] == 0;
        renderer.set_encoder(a[0], r, uint32_t(time_us_64() / 1000));
        std::puts("OK render animation");
        break;
    }
    case Command::render_stop:
        renderer.all_off();
        lights.all_off();
        std::puts("OK render stopped all-off");
        break;
    case Command::calibration_driver: {
        auto v = std::array<uint16_t, 3>{a[1], a[2], a[3]};
        if (a[0])
            calibration.tlc_gain = v;
        else
            calibration.sk_gain = v;
        renderer.invalidate();
        std::puts("OK calibration driver volatile");
        break;
    }
    case Command::calibration_encoder:
        calibration.encoder_gain[a[0]] = {a[1], a[2], a[3]};
        renderer.invalidate();
        std::puts("OK calibration encoder volatile");
        break;
    case Command::calibration_pixel:
        calibration.pixel_gain[a[0]] = {a[1], a[2], a[3]};
        renderer.invalidate();
        std::puts("OK calibration pixel volatile");
        break;
    case Command::calibration_reset:
        calibration = default_calibration();
        power = default_power_policy();
        std::puts("OK calibration reset defaults");
        break;
    case Command::calibration_export:
        std::printf(
            "CALIBRATION schema=1 status=UNMEASURED sk=%u,%u,%u tlc=%u,%u,%u "
            "minimum=%u transition=%u ceiling=%u gamma=%s\r\n",
            calibration.sk_gain[0], calibration.sk_gain[1],
            calibration.sk_gain[2], calibration.tlc_gain[0],
            calibration.tlc_gain[1], calibration.tlc_gain[2],
            calibration.minimum_assigned, calibration.transition_ms,
            calibration.global_ceiling,
            calibration.gamma == GammaProfile::quadratic ? "quadratic"
                                                         : "linear");
        break;
    case Command::calibration_ramp: {
        RenderState r;
        r.color = {255, 255, 255};
        r.magnitude = 65535;
        r.magnitude_mode = MagnitudeMode::unipolar;
        r.operational = OperationalState::assigned;
        r.temporal = TemporalMode::fade;
        r.period_ms = 2000;
        r.assigned = true;
        r.explicit_off = false;
        renderer.set_encoder(0, r, uint32_t(time_us_64() / 1000));
        std::puts("OK calibration bounded ramp encoder=0");
        break;
    }
    case Command::calibration_compare: {
        RenderState r;
        r.color = Palette::neutral;
        r.magnitude = 65535;
        r.operational = OperationalState::assigned;
        r.temporal = TemporalMode::steady;
        r.assigned = true;
        r.explicit_off = false;
        renderer.set_pixel(0, r, uint32_t(time_us_64() / 1000));
        renderer.set_encoder(0, r, uint32_t(time_us_64() / 1000));
        std::puts("OK calibration compare pixel=0 encoder=0");
        break;
    }
    case Command::too_long:
        std::puts("NGD1 type=error code=command-too-long");
        break;
    default:
        std::puts("NGD1 type=error code=unsupported-command");
        break;
    }
}
int main() {
    PicoGpio io;
    io.init_safe();
    PicoIllumination led_driver;
    led_driver.init();
    IlluminationAuthority lights;
    PowerPolicy power = default_power_policy();
    Calibration calibration = default_calibration();
    Renderer renderer;
    ElectricalFrame led_frame{};
    lights.publish(led_frame, power);
    led_driver.request(led_frame, true);
    stdio_init_all();
    std::array<PortConfig, kPorts> ports{};
    for (size_t i = 0; i < kPorts; i++)
        ports[i] = {hw::serial_data[i], harness::chain_bits[i],
                    harness::enabled[i]};
    Scanner scanner(io, ports, 250);
    Buttons buttons(button_config());
    Encoders enc(encoder_config());
    EventQueue<kEventCapacity> events;
    uint32_t sequence = 0;
    RawSnapshot snap{};
    scanner.scan(snap);
    buttons.baseline(snap);
    enc.baseline(snap);
    CommandParser parser;
    bool stream = false;
    StressState stress{};
    uint64_t diagnostic_activity = io.time_us();
    uint64_t next_scan = io.time_us() + 250,
             next_stream = io.time_us() + 100000,
             next_render = io.time_us() + 10000,
             force_tlc_by = io.time_us() + 50000;
    for (;;) {
        uint64_t now = io.time_us();
        if (now >= next_scan) {
            auto previous = snap.port;
            if (scanner.scan(snap)) {
                if (snap.scan_index > 1)
                    for (size_t port = 0; port < kPorts; ++port)
                        for (size_t bit = 0; bit < harness::chain_bits[port];
                             ++bit)
                            if (((previous[port] ^ snap.port[port]) >> bit) &
                                1u)
                                if (raw_transitions[port][bit] != UINT32_MAX)
                                    ++raw_transitions[port][bit];
                buttons.update(snap, events, sequence);
                enc.update(snap, events, sequence);
            }
            do
                next_scan += 250;
            while (next_scan <= now);
        }
        InputEvent event{};
        while (events.pop(event))
            if (stream) {
                if (event.kind == EventKind::encoder)
                    std::printf(
                        "ENCODER PHYS.%u DELTA %+d scan=%llu seq=%lu\r\n",
                        event.control, event.value, event.scan,
                        static_cast<unsigned long>(event.sequence));
                else
                    std::printf(
                        "BUTTON PHYS.%u %s scan=%llu seq=%lu\r\n",
                        event.control,
                        event.kind == EventKind::press ? "PRESS" : "RELEASE",
                        event.scan, static_cast<unsigned long>(event.sequence));
            }
        if (events.overflowed()) {
            if (input_event_overflow != UINT32_MAX)
                ++input_event_overflow;
            if (stream)
                if (std::puts("OVERFLOW input-events reconcile=snapshot") < 0 &&
                    diagnostic_output_overflow != UINT32_MAX)
                    ++diagnostic_output_overflow;
            events.clear_overflow();
        }
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT &&
            (c == '\r' || c == '\n' || (c >= 32 && c < 127))) {
            Command cmd = parser.feed(char(c));
            if (c == '\r' || c == '\n') {
                diagnostic_activity = now;
                print_command(cmd, parser.args(), snap, scanner, buttons, enc,
                              stream, lights, led_driver, led_frame, renderer,
                              calibration, power, stress, now);
            }
        }
        if (stress.running && now >= stress.ends_us) {
            stress.running = false;
            animation = Animation::none;
            lights.all_off();
            std::puts("NGD1 type=stress state=complete all_off=queued");
        }
        if (stress.running)
            diagnostic_activity = now;
        if (led_driver.enabled() &&
            now - diagnostic_activity >= kIlluminationSessionUs) {
            stress.running = false;
            animation = Animation::none;
            renderer.all_off();
            lights.all_off();
            led_driver.enable(false);
            diagnostic_activity = now;
            std::puts("NGD1 type=safety event=illumination-session-timeout "
                      "blank=asserted all_off=queued");
        }
        if (animation != Animation::none && now >= animation_next) {
            lights.all_off();
            if (animation == Animation::pixels)
                lights.set_pixel(animation_index++ % 58, {255, 0, 0});
            else if (animation == Animation::encoders)
                lights.set_encoder(animation_index++ % 8, {0, 255, 0});
            else if (animation == Animation::channels)
                lights.set_channel(animation_index++ % 24, 4095);
            else {
                Rgb colors[3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
                auto color = colors[animation_index++ % 3];
                for (size_t i = 0; i < 58; i++)
                    lights.set_pixel(i, color);
                for (size_t i = 0; i < 8; i++)
                    lights.set_encoder(i, color);
            }
            animation_next = now + 150000;
        }
        if (now >= next_render) {
            renderer.render(uint32_t(now / 1000), lights, calibration);
            do
                next_render += 10000;
            while (next_render <= now);
        }
        if (lights.publish(led_frame, power))
            led_driver.request(led_frame, led_frame.power.requested_ua ==
                                              led_frame.power.quiescent_ua);
        led_driver.service(now);
        if (led_driver.tlc_pending() &&
            (next_scan > now + 120 || now >= force_tlc_by)) {
            if (led_driver.transfer_tlc(led_frame, scanner, now))
                force_tlc_by = now + 50000;
        }
        if (stream && now >= next_stream) {
            std::printf("RAW scan=%llu", snap.scan_index);
            for (auto v : snap.port)
                std::printf(" %06lx", static_cast<unsigned long>(v));
            std::puts("");
            next_stream = now + 100000;
        }
        tight_loop_contents();
    }
}
