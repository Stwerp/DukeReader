//******************************************************************************
// MSP430F2011 Program
//
// Code for 915 MHz Transmit Board
// Test program for LEDs, Beeper, Correct LAdjust levels
//
//
//******************************************************************************

//Main Program
// Define Hardware Version:
// V1 is for the initial test run that includes the LINX module and does not require PLL programming
// V2 uses the minicircuits PLL synthesizer that requires special programming
//#define HARDWARE_V1
#define HARDWARE_V2

#include  <msp430x20x1.h>
#include "TxBoardDefinitions.h"


//Programs to Include
#ifdef HARDWARE_V1
//#include "MessageXmit.c"
#include "CWXmit.c"
//#include "TestBoard.c"
#endif // #if HARDWARE_V1

#ifdef HARDWARE_V2
#include "ProgramPLL.c"
#endif // #if HARDWARE_V2




