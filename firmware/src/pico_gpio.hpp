#pragma once
#include "neurogenic/core.hpp"
namespace ng {class PicoGpio final:public GpioBus{public:void init_safe();uint32_t read_all()override;void clock(bool)override;void load(bool)override;void delay_us(uint32_t)override;uint64_t time_us()override;};}
