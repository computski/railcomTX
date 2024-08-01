/*
 * File:   RailcomTx.c
 * Author: julian
 *
 * Created on 24 June 2024, 10:05 PM
 */

//go to Production... set configuration bits.. Generate Source Code to Output and paste here
//The 886 device does not have output latches, we need shadow registers

//Note: MCLR has to be off, else it floats and erratic behaviour results.
//LVP does not seem to work.  In theory RB3 is used but the device says it programs but then it won't run
//so stick to regular HV program mode.  Also Pickit4 does not seem to have a LV entry mode for this chip


// CONFIG1

#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTRC oscillator; port I/O function on both RA6/OSC2/CLKO pin and RA7/OSC1/CLKI pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is MCLR)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB3/PGM pin has PGM function, Low-Voltage Programming enabled)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off)
#pragma config CCPMX = RB0      // CCP1 Pin Selection bit (CCP1 function on RB0)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// CONFIG2
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal External Switchover mode disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include<pic16f88.h>
#define _XTAL_FREQ    8000000


/*
 * 8MHz internal osc
 * IOC on two pins to detect RC cutout.  Port B supports IOC
 * UART to run at 250kbps and drive one output pin, should be idle hi
 *    
 * loop waits for 20-30us RC pulse on IOC, then after delay asserts a railcom message
 * 
 * 
 * RB0 = DCC-X
 * RB1 = DCC-Y


 */

volatile union {
uint8_t                 port;
struct {
                unsigned             RA0  :1;              //RA0            
                unsigned             RA1   :1;             //RA1             
				unsigned			 RA2   :1;
                unsigned			 RA3   :1;
                unsigned             RA4  	:1;                        
                unsigned             RA5    :1;  //MCLR                         
                unsigned             RA6	:1;
                unsigned             RA7	:1;
                
                };
} sPORTA;

volatile union {
uint8_t                 port;
struct {
                unsigned             RB0  :1;              //Sense1            
                unsigned             RB1   :1;             //Sense2             
				unsigned			 RB2   :1;
                unsigned			 PGM   :1;
                unsigned             LED  	:1;                        
                unsigned             TX    :1;               //TX to line          
                unsigned             PGC	:1;
                unsigned             PGD	:1;
                
                };
} sPORTB;







void main(void) {
    OSCCONbits.IRCF=0b111;  //8MHz
    ANSEL=0;
    TRISB4=0;
    TRISB5=0;  //TX output
    
  //Timer 0
    OPTION_REGbits.T0CS=0;
    TMR0IE=1;
    PSA=1;  //1=PSA to WDT
    
    //page 157
    TXEN = 1;
    SYNC = 0;
    SPEN = 1;
    //baud rate 250kbps, this is just possible on a 8MHz internal clock
    //TX is idle high, which results in nil current in the current loop.  A zero bit is a current of 30mA
    BRGH=1;
    SPBRG=1;
  
    //WPU and IOC
    nRBPU=1;
    RBIE=1;
    
    ei();
    //OPTION_REGbits.
    
    //need a timer triggered off detection of IOC, T0 will do as we will count max 60 clock cycles
    
         
    
    while (1){
    __delay_ms(500);
    sPORTB.LED =! sPORTB.LED;
    PORTB=sPORTB.port;
    }
    
    
    
    
    return;
}


//8MHz with 0.5uS instr clock, count 256 is 128uS
//RBIE monitors PORTB<7:4> for a change, read the port to clear it. note that read-modify-write will clear it.

void __interrupt() ISR(){
  if (TMR0IF){
  TMR0IF=0;
  }
  if (RBIF){
      uint8_t x = PORTB; //read port b to clear the int condition
      RBIF=0;
  }
}
