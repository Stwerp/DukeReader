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

/*unsigned char mask2;      // 1 byte = 8 bits
unsigned short position;  // 2 bytes = 16 bits
unsigned long data;       // 4 bytes = 32 bits
*/

unsigned int count;

inline void ShortBeep()
{
  //Delay Cycles should scale with Clock speed
  //Emit a tone over the beeper
  for (unsigned int i = 500; i!=0; i--)
  {
    //Toggle Pin
    P1OUT ^= BeeperPIN;
    //Delay time
    //__delay_cycles(1000); //<- This is for 16MHz clock
    __delay_cycles(126); //<- This is for 1MHz clock
    //Turn OFF BeeperPIN
  }
  //Make Sure Beeper pin is LOW
  P1OUT &= ~(BeeperPIN);
}

inline void SleepABit()
{
  for(unsigned int count = 10000; count > 0; count--)
    __delay_cycles(40);
}
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
      //2^20 = 1048676 (21st bit)
      P1OUT &= ~(DataPIN); //Set Pin Low
    else
      P1OUT |= (DataPIN); //Set High
    //Clock in Data
    __delay_cycles(2);
    P1OUT |= ClkPIN;
    __delay_cycles(2);
    P1OUT &= ~(ClkPIN);
    //Shift Data
    PLLRegister = PLLRegister << 1;
  }
  //Sent 21 bits, Latch in Data
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
  // CP: 1 << 21 (or) 2^20 = 2,097,152
  // Control : 1 (or) 2^0 = 1
  // A : (A) << 2 (A shifted over two times)
  // B : (B) << 7 (B shifted over 7 times)
  return (unsigned long)(
    (unsigned long)(2097152)  |   //CP
    (unsigned long)(1)        |   //Control
    ((unsigned long)(A) << 2) |   //A
    ((unsigned long)(B) << 7) );  //B
}


void InitPLL()
{
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
  
  // Set P1.ALL to be correct Output/Input ports
  P1OUT = 0x00; //(clear output buffer)
  //      1111_1110 --1 for out, 0 for in
  P1DIR = (0xFF);
  P1SEL = 0x00; //set bit for special function on pin

  //Alert user that we are powered
  ShortBeep();
  
  //Sleep some Time
  SleepABit();
  SleepABit();
  SleepABit();
  
  //Initialize
  InitPLL();
  
  //Sleep some more
  SleepABit();
  SleepABit();
  SleepABit();
  
  //New Alert
  ShortBeep();
  
  //Turn on RED led and start emitting wave
  P1OUT |= LED1 + ModPIN;
  
  //Go to Full Off Mode.
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



