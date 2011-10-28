/********************************************************************
*
* Custom Definitions
*
* Stewart Thomas, Duke University
* Nov 2010
*
* Rev. 1.0, Setup
*
********************************************************************/
#ifndef PIN_DEF
#define PIN_DEF

/* MSP430 Pin Definitions */

#ifdef HARDWARE_V1
#define ModPIN    (0x01)
#define LadjPIN   (0x02)
#define PdnPIN    (0x04)
#define BeeperPIN (0x08)
#define NCPIN     (0x10)
#define LED1      (0x20)
#define LED2      (0x40)
#define LED3      (0x80)

#define LED1PIN   (0x20)
#define LED2PIN   (0x40)
#define LED3PIN   (0x80)
#endif // #if HARDWARE_V1

#ifdef HARDWARE_V2
#define ModPIN    (0x01)
#define DataPIN   (0x02)
#define ClkPIN    (0x04)
#define LePIN     (0x08)
#define BeeperPIN (0x10)
#define LED1      (0x20)
#define LED2      (0x40)
#define LED3      (0x80)

#define LED1PIN   (0x20)
#define LED2PIN   (0x40)
#define LED3PIN   (0x80)

#define LckDetPIN (0x20) //Same as LED1

//32-bit Data registers for PLL
#define FREGISTER_1   (0x96)
#define FREGISTER     (0x92)
#define RREGISTER     (0x100280)
#define NREGISTER_960     (0x112C01)

#endif // #if HARDWARE_V2



#endif // #ifndef PIN_DEF
