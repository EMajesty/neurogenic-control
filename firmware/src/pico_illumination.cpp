#include "pico_illumination.hpp"
#include "hardware_manifest.hpp"
#include "sk6812.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
namespace ng {
static PIO skpio=pio0;
bool PicoIllumination::init(){gpio_put(hw::enc_led_blank,1);gpio_put(hw::enc_led_latch,0);gpio_put(hw::enc_led_data,0);gpio_put(hw::rgb_data,0);pio_offset_=pio_add_program(skpio,&sk6812_program);sm_=pio_claim_unused_sm(skpio,true);pio_sm_config c=sk6812_program_get_default_config(pio_offset_);sm_config_set_sideset_pins(&c,hw::rgb_data);sm_config_set_out_shift(&c,false,true,24);sm_config_set_fifo_join(&c,PIO_FIFO_JOIN_TX);float div=float(clock_get_hz(clk_sys))/8000000.0f;sm_config_set_clkdiv(&c,div);pio_gpio_init(skpio,hw::rgb_data);pio_sm_set_consecutive_pindirs(skpio,sm_,hw::rgb_data,1,true);pio_sm_init(skpio,sm_,pio_offset_,&c);pio_sm_set_enabled(skpio,sm_,true);dma_=dma_claim_unused_channel(true);dma_channel_config dc=dma_channel_get_default_config(dma_);channel_config_set_transfer_data_size(&dc,DMA_SIZE_32);channel_config_set_read_increment(&dc,true);channel_config_set_write_increment(&dc,false);channel_config_set_dreq(&dc,pio_get_dreq(skpio,sm_,true));dma_channel_configure(dma_,&dc,&skpio->txf[sm_],nullptr,0,false);return true;}
void PicoIllumination::enable(bool v){if(v){enabled_=true;shutdown_=false;gpio_put(hw::enc_led_blank,0);}else{shutdown_=true;force_blank();}}
void PicoIllumination::force_blank(){gpio_put(hw::enc_led_blank,1);}
void PicoIllumination::request(const ElectricalFrame&f,bool off){stats_.requests++;sk_.request(f.sk_grb,f.generation,off);tlc_pending_=f;if(tlc_dirty_)stats_.coalesced++;tlc_dirty_=true;}
void PicoIllumination::service(uint64_t now){if(phase_==Phase::dma&&dma_channel_is_busy(dma_))return;if(phase_==Phase::dma){if(!pio_sm_is_tx_fifo_empty(skpio,sm_))return;phase_=Phase::reset;reset_until_=now+100;return;}if(phase_==Phase::reset){if(now<reset_until_)return;sk_.complete();stats_.completed_sk++;phase_=Phase::idle;if(shutdown_&&!sk_.pending()){enabled_=false;shutdown_=false;}}if(!enabled_||phase_!=Phase::idle)return;const uint8_t*d;size_t n;uint32_t gen;if(sk_.begin(d,n,gen)){for(size_t i=0;i<kSkPixels;i++)sk_words_[i]=(uint32_t(d[i*3])<<24)|(uint32_t(d[i*3+1])<<16)|(uint32_t(d[i*3+2])<<8);dma_channel_set_read_addr(dma_,sk_words_.data(),false);dma_channel_set_trans_count(dma_,kSkPixels,true);phase_=Phase::dma;}}
bool PicoIllumination::transfer_tlc(const ElectricalFrame&f,Scanner&s,uint64_t){if(!enabled_||!tlc_dirty_)return false;if(!s.acquire_led()){stats_.deferred++;return false;}struct Wire final:TlcWire{void data(bool v)override{gpio_put(hw::enc_led_data,v);}void clock(bool v)override{gpio_put(hw::scan_clk,v);}void latch(bool v)override{gpio_put(hw::enc_led_latch,v);}void half_period()override{busy_wait_at_least_cycles(8);}}wire;uint64_t begin=time_us_64();transmit_tlc(f.tlc,wire);uint64_t elapsed=time_us_64()-begin;stats_.max_tlc_us=elapsed>stats_.max_tlc_us?elapsed:stats_.max_tlc_us;stats_.completed_tlc++;tlc_dirty_=false;s.release_led(true);return true;}
}
