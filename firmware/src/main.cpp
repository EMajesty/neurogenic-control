#include "pico/stdlib.h"
#include <cstdio>
int main(){stdio_init_all();sleep_ms(1500);std::printf("NEUROGENIC controller %s protocol %d.%d\r\n",NG_FIRMWARE_VERSION,NG_PROTOCOL_MAJOR,NG_PROTOCOL_MINOR);for(;;){int c=std::getchar();if(c=='i')std::printf("identity neurogenic.reference-controller-v2 fw=%s protocol=1.3 compatibility=67-control\r\n",NG_FIRMWARE_VERSION);else if(c=='h')std::printf("commands: i identity; h help (diagnostic expansion deferred)\r\n");tight_loop_contents();}}
