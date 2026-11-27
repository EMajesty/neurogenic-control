#include "pico_gpio.hpp"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware_manifest.hpp"
namespace ng {
void PicoGpio::init_safe(){
 gpio_init(hw::scan_clk);gpio_put(hw::scan_clk,0);gpio_set_dir(hw::scan_clk,GPIO_OUT);
 gpio_init(hw::scan_load);gpio_put(hw::scan_load,1);gpio_set_dir(hw::scan_load,GPIO_OUT);
 for(auto p:hw::serial_data){gpio_init(p);gpio_set_dir(p,GPIO_IN);gpio_pull_up(p);}
 for(auto p:{hw::enc_led_data,hw::rgb_data,hw::enc_led_latch}){gpio_init(p);gpio_put(p,0);gpio_set_dir(p,GPIO_OUT);}
 gpio_init(hw::enc_led_blank);gpio_put(hw::enc_led_blank,1);gpio_set_dir(hw::enc_led_blank,GPIO_OUT);
}
uint32_t PicoGpio::read_all(){return gpio_get_all();}void PicoGpio::clock(bool v){gpio_put(hw::scan_clk,v);}void PicoGpio::load(bool v){gpio_put(hw::scan_load,v);}void PicoGpio::delay_us(uint32_t v){busy_wait_us_32(v);}uint64_t PicoGpio::time_us(){return time_us_64();}
}
