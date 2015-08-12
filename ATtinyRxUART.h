/* ATtiny84/85 USI UART v3
   This is a Receive-only UART for ATtiny84/85 that uses the hardware USI
   in three-wire (SPI) mode to receive bytes. The UART MOSI line should be 
   connected to the chip's MOSI/DI pin. MISO/DO and SCK/SCL pins are not used.
   
   Original code and description: http://www.technoblogy.com/show?VSX
   David Johnson-Davies - www.technoblogy.com - 6th May 2015
   and Edgar Bonet - www.edgar-bonet.org
   ATtiny84/85 @ 8MHz (external crystal; BOD disabled)
    OR (if OSCCAL is calibrated)
   ATtiny84/85 @ 8MHz (internal oscillator; BOD disabled)
   
   Good tutorial on AVR Timers: http://www.engblaze.com/microcontroller-tutorial-avr-and-arduino-timer-interrupts/
   Also: https://arduino-info.wikispaces.com/Timers-Arduino
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/
#ifndef ATTINYRXUART_H
#define ATTINYRXUART_H

#include <stdbool.h>
#include <core_build_options.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#if defined(__AVR_ATtiny84__)
  #define PIN_MOSI   PINA
  #define PINn_MOSI  PINA6   //Same pin as USI DI (Universal Serial Interface Data In)
  #define PCINT_MOSI PCINT6
  #define PCIEn      PCIE0
  #define PCIFn      PCIF0
  #define PCMSKn     PCMSK0
  #define TIMSKn     TIMSK0
  #define TIFRn      TIFR0
#elif defined(__AVR_ATtiny85__)
  #define PIN_MOSI   PINB
  #define PINn_MOSI  PINB0
  #define PCINT_MOSI PCINT0
  #define PCIEn      PCIE
  #define PCIFn      PCIF
  #define PCMSKn     PCMSK
  #define TIMSKn     TIMSK
  #define TIFRn      TIFR
#else
  #error Must build for ATtiny84 or ATtiny85.
#endif

#if TIMER_TO_USE_FOR_MILLIS == 0
/*This USI-based UART uses Timer0.
  The Universal Serial Interface of ATtiny84 and ATtiny85 can be configured 
  to clock to Timer0 only, so if we also want to use the standard Arduino libraries,
  we need to make sure that functions like millis(), delay(), and elapsedMillis
  all use Timer1. That leaves Timer0 free to be used here.
  The Arduino definitions for ATtiny84 ******HERE******* map those functions
  to Timer0, so it is necessary to edit the hardware definition file that defines
  TIMER_TO_USE_FOR_MILLIS so that it is set to 1. */
  #error Arduino-specified variable TIMER_TO_USE_FOR_MILLIS cannot be 0. \
  Change in the appropriate core_build_options.h, \
  possibly at Arduino\hardware\tiny\cores\tiny\core_build_options.h
#endif

#define BAUD          9600            //Fixed baud rate
#define BIT_LENGTH  ( F_CPU / BAUD )  //Number of clock cycles per bit transmission
#define BUFFER_SIZE   32              //Should be a power of 2 (for reference, SoftwareSerial has a 64-byte buffer)
#define BUFFER_MASK ( BUFFER_SIZE - 1 )

#define IS_MOSI_HIGH() (PIN_MOSI & 1<<PINn_MOSI) //Checks if MOSI pin is HIGH or LOW

// Circular buffer for storing received chars
char buffer[BUFFER_SIZE]; //TODO: remove this = (char*)malloc(BUFFER_SIZE * sizeof(char));
volatile int numBuffered = 0;  //Number of chars in buffer
volatile int startBuffer = 0;  //Index of start of buffer
volatile bool overflowed = false; //Flag for if the buffer overflows (after which, any additional received bytes are dropped)

// USI UART **********************************************

// USI receives bytes in reverse order, so we need to flip them for UART
inline unsigned char ReverseByte (unsigned char x) {
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;
}

//TODO: remove this once finished with debugging
void DbgLedToggle(int pbx) {
  DDRB |= 1<<pbx; //Output
  PINB  = 1<<pbx;
}

//Returns number of available bytes in buffer.
bool RxUART_available() {
  return numBuffered;
}

//If RxUART_available() > 0, then RxUART_read() returns the
//next buffered byte. If buffer is empty, returns 0.
char RxUART_read() {
  if(numBuffered > 0) {
    char sreg = SREG;
    cli();       //Disable interrupts
    char c = buffer[startBuffer];
    //Logical & of BUFFER_MASK performs wrap for circular buffer
    startBuffer = (startBuffer + 1) & BUFFER_MASK; //startBuffer++ plus wrap
    numBuffered--;
    SREG = sreg; //Enable interrupts
    return c;
  }
  return 0;   
}

//Tests to see if a software serial buffer overflow has occurred. 
//Calling this function clears the overflow flag.
bool RxUART_overflow() {
  if (overflowed) {
    overflowed = false;
    return true;
  }
  return false;
}

//Adds newly received byte to buffer, or sets overflow flag if buffer is full.
inline void BufferByte (char c) {
  if (numBuffered < BUFFER_SIZE) {
    //Add byte to end of buffer
    //Logical & of BUFFER_MASK performs wrap for circular buffer
    buffer[(startBuffer + numBuffered) & BUFFER_MASK] = c;
    numBuffered++;
  }
  else {
    overflowed = true;
    //Discard newly received byte
  }
}

// Initialise USI for UART reception
void InitialiseUSI (void) {  
  DDRB   &= ~(1<<PINn_MOSI);       // Define DI/MOSI as input
  USICR   = 0;                     // Disable USI
  GIFR    = 1<<PCIFn;              // Clear pin change interrupt flag
  GIMSK  |= 1<<PCIEn;              // Enable pin change interrupts
  PCMSKn |= 1<<PCINT_MOSI;         // Enable pin change on DI/MOSI pin
}

// Pin change interrupt detects start of UART reception.
ISR (PCINT0_vect) {
  if (IS_MOSI_HIGH())
    return; // Ignore if DI/MOSI is high

  GIMSK &= ~(1<<PCIEn);           // Disable pin change interrupts

  TCCR0A = 1<<WGM01;              // Timer in CTC mode
  TCCR0B = 1<<CS01;               // Set prescaler to /8
  OCR0A  = 103;                   // Shift every (103+1)*8 cycles
  TCNT0  = 206;                   // Start counting from (256-52+2)
  //This has the effect of waiting 1.5 bit-lengths for the first cycle,
  //thus skipping the start bit. Then samples happen in the middle of every bit.

  // Enable USI OVF interrupt, and select Timer0 compare match as USI Clock source
  USICR = 1<<USIOIE | 0<<USIWM0 | 1<<USICS0;
  // Clear USI OVF flag, and set USI counter to overflow after 8 received bits (at 15->0)
  USISR = 8; // 8 | 1<<USIOIF; appears unnecessary
  // After the USI receives 8 bits, ISR (USI_OVF_vect) is triggered
}

// USI overflow interrupt indicates we've received a byte
ISR (USI_OVF_vect) {
  USICR = 0;            // Disable USI
  // Before using the byte we received from USI (in USIBR), 
  // wait another bit length to test that the stop bit is high.
  TIFRn  |= 1<<OCF0A;   // Clear output compare flag
  TIMSKn |= 1<<OCIE0A;  // Enable output compare interrupt for Counter0 Match A
  // When Counter0 matches OCR0A, ISR (TIM0_COMPA_vect) is triggered
}

// Test for UART stop bit after receiving a byte
ISR (TIM0_COMPA_vect) {
  // Stop Timer/Counter0
  TCCR0A = 0;               // Set Timer0 to Normal mode
  TCCR0B = 0;               // Stop Timer0
  TIMSKn = 0;               // Disable Timer0 Interrupts

  // UART Stop bit should be HIGH
  if (IS_MOSI_HIGH()) {
    // USIBR is the buffered value from the USIDR (data register). It assembles in reverse order.
    BufferByte(ReverseByte(USIBR));
  }
  else { //Stop bit is low!
    // Check if the whole byte was low
    if(USIBR == 0) {
      //BREAK signal is a logical LOW for at least 10 bit lengths (start bit + 8 bits + stop bit)
      //TODO: Start OSCCAL synchronization mode
    }
    else {
      //THERE WAS AN ERROR
      //TODO: Figure out what to do here...
    }
  }
  GIFR   = 1<<PCIFn;        // Clear pin change interrupt flag
  GIMSK |= 1<<PCIEn;        // Enable pin change interrupts again
}

void RxUART_Begin() {
  numBuffered = 0;
  startBuffer = 0;
  InitialiseUSI();
}

#endif
