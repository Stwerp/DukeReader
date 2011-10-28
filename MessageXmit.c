//******************************************************************************
// MSP430F2011 Program
//
// Code for 915 MHz Transmit Board
//
// Transmit a user-defined message
// Flow:
// Power up
// Alert user (beep)
// Transmit CW for 10 secs
// Shift out ASK message bit by bit
// Repeat CW xmit
//
//******************************************************************************

#include  <msp430x20x1.h>
/* #include <stdlib.h> */
#include "TxBoardDefinitions.h"

/*unsigned char mask2;      // 1 byte = 8 bits
unsigned short position;  // 2 bytes = 16 bits
unsigned long data;       // 4 bytes = 32 bits
*/

/*Message Format:
 * 0xFF --> Start Bit
 * 0x00 --> Stop Bit
 * 0x0F --> Long Bit
 * 0x3F --> Short Bit
 * 0x03 --> Key Bit (Long + Short)

* Header: START, KEY, START
* Payload: Data of LONG and SHORTs
* Stop : STOP
*/
#define START   (0xFF) //Xmit On
#define STOP    (0x00) //Xmit OFF
#define LONG    (0x0F) //Xmit On 4bits
#define SHORT   (0x3F) //Xmit On 2bits
#define KEY     (0x03) //Xmit On 6bits (Long + Short)

/*unsigned char message[] = {START, KEY, START, 
    LONG, LONG, SHORT, SHORT, LONG, SHORT, LONG, SHORT, 
    LONG, LONG, SHORT, SHORT, LONG, SHORT, LONG, SHORT,
    STOP}; */
unsigned char Preamble[] = {START, KEY, START};
unsigned char CmdBEEP[] = { 0xA1, 0xB2, 0}; //Final Byte NOT Xmit'ed --String End
unsigned char CmdLED1[] = { 0x64, 0x23, 0};
unsigned char CmdLED2[] = { 0x4B, 0x32, 0};
unsigned char CmdGarbage[] = {0x5A, 0x34, 0x62, 0};
unsigned char *Msgp;    //Pointer to first byte of the command (for transmitting bits)

unsigned char *CmdArray[] = {&CmdBEEP[0], &CmdLED1[0], &CmdLED2[0], &CmdGarbage[0], 0}; //Command pointers (for selecting commands)
unsigned char CmdIndex = 0;   //Index pointer to commands

unsigned char MASK[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
//unsigned int i; //Used as a counter;
//unsigned char k; //Also a counter
unsigned char MsgByte; // used as a temp holder;


inline void Beep(unsigned int Length, unsigned char PinOut)
{
  //DEFAULT VALUES: Length = 500
  //                PinOut = BeeperPIN
  
  //Delay Cycles should scale with Clock speed
  //Emit a tone over the beeper
  for (unsigned int i = Length; i!=0; i--)
  {
    //Toggle Pin
    //P1OUT ^= BeeperPIN;
    P1OUT ^= (PinOut);
    //Delay time
    //__delay_cycles(1000); //<- This is for 16MHz clock
    __delay_cycles(126); //<- This is for 1MHz clock
    //Turn OFF BeeperPIN
  }
  //Make Sure Beeper pin is LOW
  //P1OUT &= ~(BeeperPIN);
  P1OUT &= ~(PinOut);
}

inline void AClkSleep(unsigned short Length)
{
  //DEFAULT VALUES: Length = 0xFFFF
  
  
  // Sleep
  // Initialize timerA0
  //Setup 32,768 hz crystal as SMCLK  
  CCTL0 = CCIE;                            // CCR0 interrupt enabled
  //CCR0 = 0xFFFF;                            // 32.7678 khz source * 500 ms = 16,384  cycles
  CCR0 = Length;
  BCSCTL1 &= ~(DIVA_3);                     //Clear ACLK divider
  BCSCTL1 |= DIVA_2;                        // Divide ACLK by 4
  TACTL = TACLR + TASSEL_1 + MC_1;         // Source: ACLK, Mode: UP
  
  P1OUT |= LED3;                          // Turn on Yellow LED
  // Enter LPM3 Mode
  _BIC_SR(OSCOFF); //Turn On Crystal
  _BIS_SR(CPUOFF + SCG0 + SCG1 + GIE);                 // Enter LPM3 w/ interrupts
  // -- Wake up -- //
  
  //Turn off Yellow LED
  P1OUT &= ~(LED3);
  
  //Turn Off Oscillator
  _BIS_SR(OSCOFF);
}

inline void SendByte(unsigned char message)
{
  for(int k = 8; k > 0; k--)
  {
    //Determine bit and Output
    if( (unsigned char)(message & MASK[k]) == 0)
      P1OUT &= ~(ModPIN + LED2); //0 to output (Xmit OFF)
    else
      P1OUT |= ModPIN + LED2; //1 to output (Xmit ON)
  }
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
  
  // Set P1.ALL to be correct Output/Input ports
  P1OUT = 0x00; //(clear output buffer)
  
  //      1111_1110 --1 for out, 0 for in
  P1DIR = (0xFF);
  P1SEL = 0x00; //set bit for special function on pin

  //Alert user that we are powered
  Beep(500, BeeperPIN);
  
  while(1)
  {
    //Turn on RED led and start emitting wave
    P1OUT |= LED1 + ModPIN;
    
    //Sleep for 10 sec (CW is xmitting)
    AClkSleep(0xFFFF / 2);
      
    //Transmit Message
    // Transmits MSB to LSB of message, starting from MSByte to LSByte
    // for Clock timing, go here;
    //Choose Command and store pointer to initial byte
    Msgp = CmdArray[CmdIndex++];
    if(Msgp == 0)
    {
      CmdIndex = 0;
      Msgp = CmdArray[CmdIndex];
    }
    
    if( Msgp == &CmdBEEP[0] )
    {
      //Sending a Beep command. Alert user
      Beep(200, BeeperPIN);
      //Beep(200, NCPIN);
      //Beep(100, BeeperPIN);
      //Beep(200, NCPIN);
      AClkSleep(1000);
    }
    
    //Send Preamble
    SendByte(Preamble[0]);
    SendByte(Preamble[1]);
    SendByte(Preamble[2]);
    //Send Command
    while (*Msgp != 0x00)  //for(int i = 0; i < 2; i++)
    {
      MsgByte = *Msgp++;
      for(int k = 8; k > 0; k--)
      {
        if( (unsigned char)(MsgByte & MASK[k]) == 0)
          SendByte(LONG); //Bit is 0
        else
          SendByte(SHORT); //Bit is 1
      }
    }
    //Send STOP
    SendByte(STOP);
    
    
    //Turn off LED2
    P1OUT &= ~(LED2);
  }//End Loop
       
}//End Main

/*inline void SendMessage(unsigned char[] message)
{
  unsigned int i; //Used as a counter;
  //unsigned char k; //Also a counter
  unsigned char MsgByte; // used as a temp holder;
  
 i = 0;
 while(message[i] != 0x00)
  {
    MsgByte = (unsigned char)message[i];
    for(int k = 8; k > 0; k--)
    {
      //Determine bit and Output
      if( (unsigned char)(MsgByte & MASK[k]) == 0 )
        P1OUT &= ~(ModPIN + LED2); //0 to output (Xmit OFF)
      else
        P1OUT |= ModPIN + LED2; //1 to output (Xmit ON)
    }
    i++;
  }
  //Turn off LED2
  P1OUT &= ~(LED2);
  //Restore Clock timing;
} //End SendMessage */


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



