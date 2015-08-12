# ATtiny Rx-only UART
ATtiny84/85 Receive-only Serial UART using the USI hardware

This is a compact and efficient receive-only Serial UART for the ATtiny84 and ATtiny85,
based on the work by David Johnson-Davies and Edgar Bonet. See the original work at 
http://www.technoblogy.com/show?VSX. It has been tested with [arduino-tiny](https://code.google.com/p/arduino-tiny/), 
an ATtiny core for Arduino[^timer], on ATtiny84 and ATtiny85 microcontrollers. It may work on other ATtinys
with little to no modification, but that hasn't been tested[^untested].

Compared to the [original version](http://www.technoblogy.com/show?VSX), the following improvements have been made:
- The ATtiny84 and 85 code base is combined.
- For use with Arduino, it checks that TIMER_TO_USE_FOR_MILLIS != 0, because the USI needs to use Timer0[^timer].
- After reading a byte, it checks that the UART Stop bit is high before proceeding.
- Received bytes are written to a buffer that can be polled and read from, similar to regular Serial libraries.

This is a Receive-only UART for ATtiny84/85 that uses the hardware USI in three-wire (SPI) 
mode to receive bytes. The UART MOSI line should be connected to the ATtiny's MOSI/DI pin. 
MISO/DO and SCK/SCL pins are not used. Timer/Counter0 is used by the USI to read in bytes.[^timer]


[^timer]: For use with ATtiny84, you must first edit Arduino\hardware\tiny\cores\tiny\core_build_options.h 
so that TIMER_TO_USE_FOR_MILLIS == 1 for __AVR_ATtinyX4__ processors. By default, it is set to 0, but the USI
(which this code uses) requires Timer0 to clock against. If you are programming in pure C (no Arduino libraries),
just be aware that Timer0 must be reserved for this code.

[^untested]: If you can verify that it works on any other model, please let me know, or submit a 
Pull Request if any code changes were required.