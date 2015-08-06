/* ATtiny84/85 USI UART v3
   This is a Receive-only UART for ATtiny84/85 that uses the hardware USI
   in three-wire (SPI) mode to receive bytes. The UART MOSI line should be 
   connected to the chip's MOSI/DI pin. MISO/DO and SCK/SCL pins are not used.

   Original code and description: http://www.technoblogy.com/show?VSX
   David Johnson-Davies - www.technoblogy.com - 6th May 2015
   and Edgar Bonet - www.edgar-bonet.org
   ATtiny84/85 @ 8MHz (external crystal; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#if defined(__AVR_ATtiny84__)
  #define ATTINY_PIN        PINA
  #define ATTINY_PIN_MOSI   PINA6
  #define ATTINY_PCINT_MOSI PCINT6
  #define ATTINY_PCIE       PCIE0
  #define ATTINY_PCIF       PCIF0
  #define ATTINY_PCMSK      PCMSK0
#elseif defined(__AVR_ATtiny85__)
  #define ATTINY_PIN        PINB
  #define ATTINY_PIN_MOSI   PINB0
  #define ATTINY_PCINT_MOSI PCINT0
  #define ATTINY_PCIE       PCIE
  #define ATTINY_PCIF       PCIF
  #define ATTINY_PCMSK      PCMSK
#else
  #error Must build for ATtiny84 or ATtiny85.
#endif

// Constant
const int DataIn = ATTINY_PIN_MOSI;   // USI DI

// USI UART **********************************************

unsigned char ReverseByte (unsigned char x) {
  x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
  x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
  x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
  return x;    
}

// Initialise USI for UART reception.
void InitialiseUSI (void) {  
  DDRB &= ~(1<<DataIn);           // Define DI as input
  USICR = 0;                      // Disable USI.
  GIFR = 1<<ATTINY_PCIF;                 // Clear pin change interrupt flag.
  GIMSK |= 1<<ATTINY_PCIE;               // Enable pin change interrupts
  ATTINY_PCMSK |= 1<<ATTINY_PCINT_MOSI;  // Enable pin change on DI/MOSI pin
}

// Pin change interrupt detects start of UART reception.
ISR (PCINT0_vect) {
  if (ATTINY_PIN & 1<<DataIn) return;    // Ignore if DI is high
  GIMSK &= ~(1<<ATTINY_PCIE);     // Disable pin change interrupts
  TCCR0A = 2<<WGM00;              // Timer in CTC mode
  TCCR0B = 2<<CS00;               // Set prescaler to /8
  OCR0A = 103;                    // Shift every (103+1)*8 cycles
  TCNT0 = 206;                    // Start counting from (256-52+2)
  // Enable USI OVF interrupt, and select Timer0 compare match as USI Clock source:
  USICR = 1<<USIOIE | 0<<USIWM0 | 1<<USICS0;
  USISR = 1<<USIOIF | 8;          // Clear USI OVF flag, and set counter
}

// USI overflow interrupt indicates we've received a byte
ISR (USI_OVF_vect) {
  USICR = 0;                      // Disable USI         
  int temp = USIDR;
  Display(ReverseByte(temp));
  GIFR = 1<<ATTINY_PCIF;          // Clear pin change interrupt flag.
  GIMSK |= 1<<ATTINY_PCIE;        // Enable pin change interrupts again
}

void Display (char c) {
  // Process received byte here
}

// Main **********************************************

void setup (void) {
  InitialiseUSI();
}

void loop (void) {
}