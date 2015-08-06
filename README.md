# ATtiny Rx-only UART
ATtiny84/85 Receive-only Serial UART using the USI hardware

This is a compact and efficient receive-only Serial UART for the ATtiny84 and ATtiny85,
based on the work by David Johnson-Davies and Edgar Bonet. See the original work at 
http://www.technoblogy.com/show?VSX. It has been adapted such that the ATtiny84 and 85
code base is combined.

This is a Receive-only UART for ATtiny84/85 that uses the hardware USI in three-wire (SPI) 
mode to receive bytes. The UART MOSI line should be connected to the chip's MOSI/DI pin. 
MISO/DO and SCK/SCL pins are not used.