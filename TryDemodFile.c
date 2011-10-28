//******************************************************************************
// MSP430F2011 Program
//
// Passive Data Demodulator
//
//  Detects ASK/OOK data. Time between RF breaks indicate digital data
//
//******************************************************************************

#include  <msp430x20x1.h>
#include "./SmartHatDefinitions.h"

// PreProcessor Definitions

// Slowest Clock
// Turn DCO to slowest clock (method from Errata sheet) (line 1)
// 0b1111_0000 -> Clear out previous setting (Set RSEL bits) (line 2)
// Place new setting for RSEL : 0b0000_xxxx (line 3)
#define SLOW_CLOCK  \
  DCOCTL = 0x00;   \
  BCSCTL1 &= 0xF0;  \
  BCSCTL1 |= 0x00;     
  //DCOCTL |= 0x00;      // Place new setting for DCO  : 0bxxx0_0000

// Receive Clock (3.5 MHz)
// Turn DCO to slowest clock (method from Errata sheet) (line 1)
// 0b1111_0000 -> Clear out previous setting (Set RSEL bits) (line 2)
// Place new setting for RSEL : 0b0000_xxxx (line 3)
#define RECEIVE_CLOCK \
  DCOCTL = CALDCO_1MHZ; \
  BCSCTL1 = CALBC1_1MHZ;
  //DCOCTL |= 0x00;      // Place new setting for DCO  : 0bxxx0_0000
  
//Global Variables
/*unsigned char mask2;      // 1 byte = 8 bits
unsigned short position;  // 2 bytes = 16 bits
unsigned long data;       // 4 bytes = 32 bits
*/
unsigned char Buffer[16]; //Receive buffer (16 bytes)
unsigned char *Buffp; //Pointer to receive buffer
unsigned char RcvByte; //Temp receiving byte
unsigned short Count, TRCAL, Threshold; //Count = read from TAR, TRCAL, demod threshold
unsigned char BitFlag; //Flag for first bit received
unsigned char NumBits = 0; //Number of Bits received (used in Comparator isr)
  
  
//Function Definitions
/*extern inline void setup_a_clk();
extern inline void sleep();
extern inline void beep();
extern inline void FlashLED(unsigned char LED);
extern inline void XmitOut(unsigned char* ptr, unsigned char Pin); */

// *** Setup A_CLK
// *** sets ACLK to be used for sleeping

// *** Sound tone on beeper
inline void beep()
{
  // Generate 5.0 kHz tone -- Tone 1
  for ( unsigned short i = 175 ; i != 0 ; i-- )
  {
    // Toggle Pin ON
    P1OUT = BeeperPIN;//0x04;
    // Delay for 1/2 period of 2.5 kHz (10 cycles)
    //__delay_cycles(1);
    
    // Toggle Pin OFF
    P1OUT = 0x00;//LED1PIN; //BeeperPIN; <-- Changed temporarily
    // Delay
    __delay_cycles(2);
  }
}

// *** Flash LED
inline void FlashLED(unsigned char LED)
{
  // Generate 5.0 kHz tone -- Tone 1
  for ( unsigned short i = 50 ; i != 0 ; i-- )
  {
    // Toggle Pin ON
    P1OUT = LED;
    // Delay for 1/2 period of 2.5 kHz (10 cycles)
    //__delay_cycles(1);
    
    // Toggle Pin OFF
    P1OUT = 0x00;//LED1PIN; //BeeperPIN; <-- Changed temporarily
    // Delay
    __delay_cycles(2);
  }
}

// *** Transmit a message on a P1 pin (or pins)
inline void XmitOut(unsigned char* ptr, unsigned char Pin)
{
  //Outputs a message on a pin MSB to LSB
  //0 is LONG
  //1 is SHORT
  unsigned char MASK[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  
  while (*ptr != 0x00){  
    for(int k = 8; k > 0; k--){
      P1OUT |= (Pin); //Pulse high
      if( (unsigned char)(*ptr & MASK[k]) )
        __delay_cycles(10); //SHORT pulse -- bit is high
      else
        __delay_cycles(20); //LONG pulse -- bit is low
      P1OUT &= ~(Pin); //Return low
    }//End For
    ptr++;
  }//End While
}//End XmitOut
  
    
void main (void) //Change to Loop
{
  //Set Clock to slowest setting
  DCOCTL = 0x00;   \
  BCSCTL1 &= 0xF0;  \
  BCSCTL1 |= 0x00;  
  
  BCSCTL2 |= DIVM0; //Divide MCLK by 2
  WDTCTL = WDTPW + WDTHOLD;   // Stop watchdog timer
  // Turn OFF SMCLK and external Oscillator
  _BIS_SR(OSCOFF + SCG1);
  
  // Set P1.ALL to be Output ports and turn OFF special pin functions
  P1OUT = 0x00;
  CAPD = CAPD1 + CAPD0; //Disable port connections (buffers)
  P1DIR = 0xFC; // 0x1111_1100
  P1SEL = 0x00;

  while(1)
 {
  //Setup ACLK to 10kHz source (VLO)
  BCSCTL3 |= LFXT1S_2;
  // Setup ACLK to be dvided by 8
  BCSCTL1 |= DIVA1 + DIVA0; 
  //Sleep to let Storage caps charge up.
  CCR0 = 25;                              // 10 khz source * 100 ms = 1,000  cycles
  TACTL = TACLR + TASSEL_1 + MC_1;         // Source: ACLK, Mode: UP
  // Enter LPM3 Mode
  _BIC_SR(OSCOFF);
  CCTL0 = CCIE;                            // CCR0 interrupt enabled
  _BIS_SR(LPM3_bits + GIE);                // Enter LPM3 w/ interrupts
  // -- Wake up -- //
  // Turn OFF external Oscillator
  _BIS_SR(OSCOFF);
  // Turn OFF Timer Interrupts
  //CCTL0 = 0x00;
  
  
  
  //-- Debug --//
  FlashLED(LED3);
 }
 {
  //Receive Mode
  // Set MCLK to be 1 MHz div by 4
  // Source Timer A from MCLK
  // Start Timer A in UP mode (set CCR0 to 0xFFFF(MAX))
  // Setup comparator to falling edge
  // Enter LPM3;
  
  //Comparator Interrupt:
  // Read and Store TACount
  // Reset TACount
  // On Falling Edge:
  //    Reset to read on Rising Edge
  // On Rising Edge:
  //    Determine bit number and create or compare to stored threshold
  //    0/1 bit decision and shift into data register
  //    On full Byte, shift data storage pointer
  //    On full command / count timeout, RETI
  
  //Receive Mode (cont.)
  // Return clock to SLOW mode
  // Process received command (Beep, LED, etc.)
  // Clean up, and return to waiting commands
  //END
  
  //Init Variables
  BitFlag = 0;          //Reset Flag
  Buffp = &Buffer[0];   //Point to start of receive buffer
  NumBits = 0;          //Reset to 0 bits received
  
  
  //Set MCLK to be DCO DIV-by-8 and SMCLK to be DCO DIV-by-4
  BCSCTL2 = DIVM_3 + DIVS_2;
  //Source TimerA from SMCLK (DCO / 4) in Continuous mode
  TACTL = TASSEL_2 + MC_2; //(MC_1 = UP MODE, MC_2 = Continuous Mode)
  //TACCR0 = 0xFFFF;
  //Clear TIMER A Flag
  //TACCTL1 = CCIFG;
  //TAR = TimerA Count register(16bits)
  //Turn On Comparator-- (Change these for our pins)
  CACTL2 = P2CA0 + P2CA1 + CAF; //Use CA0 and CA1, filter on
  CACTL1 = CAEX; //Set P1.0 to be reference(-) and P1.1 to be data(+)
  CAPD = CAPD1 + CAPD0; //Disable port connections (buffers)
  //Enable Comparator Interrupts (on Falling Edge)
  CACTL1 |= CAON + CAIES + CAIE;  //Comp. on, Falling edge, Interrupt enabled
  //Setup DCO to be 1 MHZ / 8
  RECEIVE_CLOCK;
  //Enter LMP3; -- Can only turn off CPU (& oscillator), since we need DCO and SMCLK running
  _BIC_SR(SCG1); //SMCLK turn ON (for TimerA system)
  _BIS_SR(CPUOFF + GIE);                // Enter LPM w/ interrupts
  //Turn OFF
  //(... Comparator interrupt handling bit receiving ...)//
  // -- Wake up -- //
  
  //Disable SMCLK, Oscillator, and Interrupts
  _BIC_SR(SCG1 + OSCOFF + GIE);
  //Return clock to slowest setting
  SLOW_CLOCK;
  BCSCTL2 |= DIVM0; //Divide MCLK by 2
  //Turn OFF Watchdog Timer
  WDTCTL = WDTPW + WDTHOLD;
  //Turn OFF Comparator
  TACTL = 0x00;
  //Turn OFF TimerA
  TACTL = 0x00;
  
  //-- Debug --//
  //Output received message
  XmitOut(&Buffer[0], NCPIN);
  
  //Process Command (Beep, LED, etc.)
  if( (Buffer[0] == 0xA1) && (Buffer[1] == 0xB2) )
    //beep();
    beep();
  else if( (Buffer[0] == 0x64) && (Buffer[1] == 0x23) )
    //Light LED1
   FlashLED(LED1);
  else if( (Buffer[0] == 0x4B) && (Buffer[1] == 0x32) )
    //Light LED2
    FlashLED(LED2);
  else
    //No recognized command, error
    FlashLED(LED3);
  //End Command Processing
  
  //Clean up and return to waiting for a command
 }

  // -- -- -- -- //
} // Close Program


// *** Timer A0 interrupt service routine
// *** used for returning from sleep
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
{
  //Return from Sleep
  //
  // Turn timer interrupts OFF
  CCTL0 = 0x00;
  // Turn OFF Timer
  TACTL = 0x00;
  // Turn CPU on
  _BIC_SR_IRQ(LPM3_bits + GIE);
}

// *** Comparator A+ interrupt service routine
// *** Processes receiving and demodulating bits
#pragma vector=COMPARATORA_VECTOR
__interrupt void COMPARATORA (void)
{
  //Comparator Interrupt:
  // Read and Store TACount
  // Reset TACount
  // On Falling Edge:
  //    Reset to read on Rising Edge
  // On Rising Edge:
  //    Determine bit number and create or compare to stored threshold
  //    0/1 bit decision and shift into data register
  //    On full Byte, shift data storage pointer
  //    On full command / count timeout, RETI
  
  //Store current timer value
  Count = TAR;
  //Clear/reset timer
  TACTL |= TACLR;
  //Check which state we were in
  if( ~(CACTL1 & CAIES) )
  {//Entered from RISING EDGE    
    if( BitFlag ){
      //More than 1 bit received
      if(Count > TRCAL){
        //STOP Byte
        //Store Stop Byte into message buffer
        // and return from demodulation.
        *Buffp = 0x00;
        _BIC_SR_IRQ(CPUOFF + GIE);  //on Exit, turn ON cpu and disable interrupts
      }else{
        //Bit Desicion and storing
        RcvByte <<= 1; //Left shift by 1
        RcvByte |= (Count < Threshold); //TRUE for SHORT bit, and is 0x01
                                    //FALSE for LONG bit, and is 0x00
        //If 8 bits have been received, store byte and move pointer
        if(NumBits++ == 7){
          *Buffp++ = RcvByte;
          NumBits = 0;
        }
      }//End Count > TRCAL
    }else{
      //First Bit -- store TRCAL, Threshold
      TRCAL = Count;
      Threshold = TRCAL / 2;
      BitFlag = 0xFF;
    }
  }
  CACTL1 ^= CAIES;  //Change edge mode (Rise/Fall, and Fall/Rise)
  //Start/Stroke the dog (Reset Watchdog Timer)
  WDTCTL = WDTPW + WDTCNTCL;
}

