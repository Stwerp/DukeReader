//******************************************************************************
// MSP430F2011 Program
//
// Code for 915 MHz Transmit Board
// Test program for LEDs, Beeper, Correct LAdjust levels
//
//
//******************************************************************************

#include  <msp430x20x1.h>
/* #include <stdlib.h> */
#include <TxBoardDefinitions.h>

/*unsigned char mask2;      // 1 byte = 8 bits
unsigned short position;  // 2 bytes = 16 bits
unsigned long data;       // 4 bytes = 32 bits
*/

void ShortBeep()
{
  //Delay Cycles should scale with Clock speed
  //Emit a tone over the beeper
  for (unsigned int i = 500; i!=0; i--)
  {
    //Toggle Pin
    P1OUT ^= BeeperPIN;
    //Delay time
    //__delay_cycles(1000); //<- This is for 16MHz clock
    __delay_cycles(63); //<- This is for 1MHz clock
  }
  P1OUT = 0;
}

void AClkSleep()
{
  // Sleep
  // Initialize timerA0
  //Setup 32,768 hz crystal as SMCLK  
  CCTL0 = CCIE;                            // CCR0 interrupt enabled
  CCR0 = 16384;                            // 32.7678 khz source * 500 ms = 16,384  cycles
  TACTL = TACLR + TASSEL_1 + MC_1;         // Source: ACLK, Mode: UP
  // Enter LPM3 Mode
  _BIC_SR(OSCOFF); //Turn On Crystal
  _BIS_SR(LPM3_bits + GIE);                 // Enter LPM3 w/ interrupts
  // -- Wake up -- //
  
  //Turn Off Oscillator
  _BIS_SR(OSCOFF);
}


void main(void)
{
  // Turn DCO to slowest clock (method from Errata sheet)
  DCOCTL = 0x00;
  // Set RSEL bits
  BCSCTL1 &= 0xF0;    // 0b1111_0000 -> Clear out previous setting
  //BCSCTL1 |= (XTS);    // Place new setting for RSEL : 0b0000_xxxx
  DCOCTL = CALDCO_1MHZ_;     // Place new setting for DCO  : 0bxxx0_0000*/
  BCSCTL1 = CALBC1_1MHZ_; //Setup pre-calibrated 1 MHz clock

  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  
  // Turn OFF SMCLK and external Oscillator
  _BIS_SR(OSCOFF + SCG1 + SCG0); // (leave CPU clock on)
  
  // Set P1.ALL to be correct Output/Input ports
  P1OUT = 0x00; //(clear output buffer)
  //      1111_1110 --1 for out, 0 for in
  P1DIR = (0xFF);
  P1SEL = 0x00; //set bit for special function on pin

  while (1)
 {
   // // // // // // // // // // // // //
   // // // Begin Main Loop Code // // //
   // // // // // // // // // // // // //
   
   //Short Beep
   //ShortBeep();
   
      
   
 }  
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
  _BIC_SR_IRQ(CPUOFF + GIE);
}



