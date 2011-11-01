//******************************************************************************
// MSP430F2011 Program
//
// Code for 915 MHz Transmit Board
//
// Program the Mini-circuits PLL / frequency synthesizer
//
//******************************************************************************

#include  <msp430x20x1.h>
/* #include <stdlib.h> */
#include "TxBoardDefinitions.h"

/* Not used, just here to remind me how long shorts and longs are */
/*unsigned char mask2;      // 1 byte = 8 bits
unsigned short position;  // 2 bytes = 16 bits
unsigned long data;       // 4 bytes = 32 bits
*/

unsigned int count;

inline void ShortBeep()
{
  //The Delay Cycles should scale with Clock speed, but I don't have the equations for them.
  //Emit a tone over the beeper -- This is just intended for user feedback.
  for (unsigned int i = 500; i!=0; i--)
  {
    //Toggle Pin
    P1OUT ^= BeeperPIN;
    //Delay time
    //__delay_cycles(1000); //<- This is for the 16MHz clock
    __delay_cycles(126); //<- This is for the 1MHz clock
    //Turn OFF BeeperPIN
  }
  //Make Sure Beeper pin is LOW when we exit
  P1OUT &= ~(BeeperPIN);
}

/* Delay program execution using clock cycles */
inline void SleepABit()
{
  for(unsigned int count = 10000; count > 0; count--)
    __delay_cycles(40);
}
/* Delay program execution using sleeping and onboard clock */
inline void AClkSleep()
{
  // Sleep
  // Initialize timerA0
  //Setup 32,768 hz crystal as SMCLK  
  CCTL0 = CCIE;                            // CCR0 interrupt enabled
  CCR0 = 16384;                            // 32.7678 khz source * 500 ms = 16,384  cycles
  TACTL = TACLR + TASSEL_1 + MC_1;         // Source: ACLK, Mode: UP
  // Enter LPM3 Mode
  _BIC_SR(OSCOFF); //Turn On Crystal
  _BIS_SR(CPUOFF + SCG0 + SCG1 + GIE);                 // Enter LPM3 w/ interrupts
  // -- Wake up -- //
  
  //Turn Off Oscillator
  _BIS_SR(OSCOFF);
}

//Clock Out Data to the PLL
void ClockOutData( unsigned long DataRegister )
{
  unsigned long PLLRegister = (unsigned long)(DataRegister);
  //Program the PLL
  // Model: Analog ADF4118
  // Clocked in using Rising Clock edge, MSB First, Rising Edge of LE locks in data
  //Register Values:
  //F Register: (MSB) X0XXX00000X00100100_10 (LSB) 
  //N Register: (MSB) 1_0001001011000_00000_01 (LSB)
  //R Register: (MSB) 1_XXXX_0000001.0100000_00 (LSB)
  
  
  //Make sure clock is low
  P1OUT &= ~(ClkPIN);
  for(unsigned char i = 21; i > 0; i--){
    //Test 21st Bit and Set Data pin accordingly
    if((PLLRegister & ((unsigned long)(1048576))) == 0) 
      //2^20 = 1048576 (21st bit) == 0x100000
      P1OUT &= ~(DataPIN); //Set Pin Low
    else
      P1OUT |= (DataPIN); //Set High
    //Clock in Data (raise high, then bring low)
    __delay_cycles(2);
    P1OUT |= ClkPIN;
    __delay_cycles(2);
    P1OUT &= ~(ClkPIN);
    //Shift Data
    PLLRegister = PLLRegister << 1;
  }
  //After sending 21 bits, Latch in Data
  __delay_cycles(2);
  P1OUT |= LePIN;
  __delay_cycles(2);
  P1OUT &= ~(LePIN);  
}

unsigned long FormNReg( unsigned char A, unsigned short B )
{
  //Assumes channel spacing of 50kHz (8 MHz / 160) in R Regsiter
  //Returns a register for programming the N Register
  //FORMAT (21 bits total):
  //  [1b][13b][5b][2b]
  //  [(1)CP Gain][B counter][A counter][(01)Control Bits]
  //  CP      : 1 << 21 (or) 2^20 = 2,097,152
  //  B       : (B) << 7 (B shifted over 7 times)
  //  A       : (A) << 2 (A shifted over two times)
  //  Control : 1 (or) 2^0 = 1
  //(Details on the Analog Devices spec sheet are somewhat fuzzy as to exactly what
  //  these values mean. These values were ripped from a test program available from
  //  minicircuits for the synthesizer we are using [which has an internal Analog Devices PLL]
  //  set for 915 MHz output.)
  return (unsigned long)(
    (unsigned long)(2097152)  |   //CP
    (unsigned long)(1)        |   //Control
    ((unsigned long)(A) << 2) |   //A
    ((unsigned long)(B) << 7) );  //B
}


void InitPLL()
{
  //Initializes the PLL for 915 MHz output. The numbers were found from a program available at
  //  minicircuits website for the synthesizer we are using. (Slightly) More details are available 
  //  from the analog devices pll (internal to the minicircuits frequency synthesizer) datasheet.
  
  //Initalizes PLL for 915MHz output
  ClockOutData( (unsigned long)(FREGISTER_1) );
  ClockOutData( (unsigned long)(RREGISTER) );
  //ClockOutData( (unsigned long)(NREGISTER_960) );
  ClockOutData( (unsigned long)(FormNReg( (unsigned char)(28), (unsigned short)(571))) );
  ClockOutData( (unsigned long)(FREGISTER) );
}


void main(void)
{
  // Turn DCO to slowest clock (method from Errata sheet)
  DCOCTL = 0x00;
  // Set RSEL bits
  BCSCTL1 &= 0xF0;    // 0b1111_0000 -> Clear out previous setting
  //BCSCTL1 |= (XTS);    // Place new setting for RSEL : 0b0000_xxxx
  DCOCTL = CALDCO_1MHZ;     // Place new setting for DCO  : 0bxxx0_0000*/
  BCSCTL1 = CALBC1_1MHZ; //Setup pre-calibrated 1 MHz clock

  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  
  // Turn OFF SMCLK and external Oscillator
  _BIS_SR(OSCOFF + SCG1); // (leave CPU clock on)
  
  // Set P1.(ALL) to be correct Output/Input ports
  P1OUT = 0x00; //(clear output buffer)
  //      1111_1111 --1 for out, 0 for in
  P1DIR = (0xFF);
  P1SEL = 0x00; //set bit for special function on pin (1 for special fn, 0 for normal operation)

  //Alert user that we are powered
  ShortBeep();
  
  //Sleep some Time
  SleepABit();
  SleepABit();
  SleepABit();
  
  //Initialize
  InitPLL();
  
  //Sleep some more -- This is required so that the the PLL can lock onto its frequency and we
  //  don't turn on the power amp too soon. That would make the amp attempt to draw too much
  //  current and blow the fuse. If this were working "properly", we would wait until the PLL
  //  responds that it has locked onto the frequency of interest before continuing. Until then,
  //  though, just waiting works perfectly fine.
  SleepABit();
  SleepABit();
  SleepABit();
  
  //New Alert -- Now, we're getting ready to send out power!
  ShortBeep();
  
  //Turn on RED led and start emitting wave
  P1OUT |= LED1 + ModPIN;
  
  //Go to Full Off Mode. This leaves the amp on and sending out a CW wave.
  LPM4;
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
{
  // Turn timer interrupts OFF
  CCTL0 = 0x00;
  // Turn OFF Timer
  TACTL = 0x00;
  // Turn CPU on
  _BIC_SR_IRQ(CPUOFF + SCG0 + GIE);
}



