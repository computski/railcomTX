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

//uS time points through the cutout
#define T_CS    32U
#define T_TS1   80U
#define T_TS2   193U
#define T_CE    455U


enum states{
    S_DCC_BIT,
    S_CUTOUT_START,
    S_CHANNEL1,
    S_CHANNEL2,
   
};

uint8_t deviceState = S_DCC_BIT;

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
                unsigned             RA0  :1;              //RA0 output test point            
                unsigned             RA1   :1;             //RA1 output test point            
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

//hardware ERROR!  only RB4-7 support IOC.  grr.  So I will need to use PGC and PGD, but this in turn means I cannot debug the thing using pickit
//TX is locked to RB4.  Bummer.
//Solution: Use INT instead, this is RB0.  Can set it as neg edge triggered and then sample for both RB0,1 being at zero

//but TWO issues with this.  first we might see what is the front edge of the RC preamble because the 'sense' of the signal is inverted
//second is the waveform takes a long time to decay to zero so we won't see both at zero for quite some time.

//sense A
//X is falling (X is blue). This is the falling edge of the prior bit, and now the Y line is rising at the start of the RC-flag bit
//we won't see another falling edge until the end of the RC cutout, or possibly one DCC half bit later
//


//sense B
//X is falling (X is red))
//This is the falling edge end of the RC-flag bit.  We won't see another falling edge until the bit stream returns.


//so this is a bit like DCC decoding off INT.  that works because the signal is symetric so you don't care whether you are on X or Y
//with RC you do care, because its asymetric.  the RC pulse might be rising edge on X followed 20uS later by falling edge.
//or it might not be present at all because if its on Y, INT won't see it.  INT sees a falling edge and then nothing more, it stays low.



volatile    uint8_t bothLow;




void main(void) {
    OSCCONbits.IRCF=0b111;  //8MHz
    ANSEL=0;
    PORTA=sPORTA.port;
    TRISB4=0;
    TRISB5=0;  //TX output
    TRISA0=0;  //debug output
    
  //Timer 0
    OPTION_REGbits.T0CS=0;
    TMR0IE=0; //do not enable T0 ints
    PSA=0;  //1=PSA to WDT, 0 = PSA to TMR0 giving div2

//timer 1. Boot default is instruction clock, no prescale
    TMR1CS=0;
    TMR1ON=1;
    TMR1IE=1;
    PEIE=1; //peripheral ints


    
    //page 157
    TXEN = 1;
    SYNC = 0;
    SPEN = 1;
    //baud rate 250kbps, this is just possible on a 8MHz internal clock
    //TX is idle high, which results in nil current in the current loop.  A zero bit is a current of 30mA
    BRGH=1;
    SPBRG=1;
  
    //WPU and IOC
//    nRBPU=1;
 //   RBIE=1;
   
    //INT config
    //boot, looking for a rising edge
    
    INTEDG = 1;
    INT0IE=1;
    
    
    ei();
    //OPTION_REGbits.
    
    //need a timer triggered off detection of IOC, T0 will do as we will count max 60 clock cycles
    
         
    
    while (1){
    //__delay_ms(500);
    //sPORTB.LED =! sPORTB.LED;
    //PORTB=sPORTB.port;
    NOP();
    
    //to detect 
    
    
    
    
    
    }
    
    
    
    
    return;
}


//8MHz with 0.5uS instr clock, count 256 is 128uS
//RBIE monitors PORTB<7:4> for a change, read the port to clear it. note that read-modify-write will clear it.

void __interrupt() ISR(){
  if (TMR0IF){
 TMR0IF=0;
 return;
      switch(deviceState){
          case S_CUTOUT_START:
              //we hit timeout, its not an RC cutout
              deviceState=S_DCC_BIT;
              //Disable timer0 and look for a rising edge
              TMR0IE=0;
              INTEDG=1;
              INTE=1;
              break;
          
          case S_CHANNEL1:
              //send Ch1 bits, set timer to trigger at start of Ch2
              TMR0=255U-T_TS2-T_TS1;
              deviceState=S_CHANNEL2;
              break;
              
          case S_CHANNEL2:
              //send Ch2 bits, set timer to look for end of RC
              TMR0=255U-T_CE-T_TS2;
              deviceState=S_DCC_BIT;
              break; 
              
          case S_DCC_BIT:
              //have reached end of cutout
              //look for rising edge again
              INTEDG=1;
              INTE=1;
              TMR0IE=0;
              
      }
      
      TMR0IF=0;
  }
  
  if (TMR1IF){
            TMR1IF=0;
    sPORTB.LED =! sPORTB.LED;
        PORTB=sPORTB.port;
        TMR1H=0x01;
        TMR1L=0;
  }
  
  
  //using a state engine is relatively slow. would be faster to test whether we are looking for rising/falling edge
  //and if falling we know this is the start of the rc cutout
  //and on rising, we can set T0= ch1 transmit point.  then we either see falling and test ok on XY and allow T0 to trigger the ch1
  //else we don't see falling at the point we hit T0 and this resets
  //mind you, a 1-bit half is 57uS so we might see falling from this before our T0 timeout.  so better we check the T0 value at this point
  //as we were gunning for 80uS
  
  if (INT0IF){
      switch(deviceState){
          case S_DCC_BIT:
              //set timer for T_CS with 5uS overhead
              TMR0=255-T_CS+5;
             //found rising edge, look for falling
              INTEDG=0;
              sPORTA.RA0=0;
              PORTA=sPORTA.port;
              deviceState=S_CUTOUT_START;
              TMR0IE=1;
              break;
          case S_CUTOUT_START:
              //found falling edge, assume both X&Y are low
              if ((PORTB & 0b11)==0){
                  sPORTA.RA0=1;
                  TMR0=255-T_TS1-T_CS-5;
                  //deviceState=S_CHANNEL1; //queue up ch1
                   deviceState=S_DCC_BIT; //debug
                  INTEDG=1; //debug, else 0
              }else{
                  //not an RC cutout
                  sPORTA.RA0=0;
                  INTEDG=1;  //default to looking for rising edge              
                    INTE=1;
                     deviceState=S_DCC_BIT; 
              }
              
                PORTA=sPORTA.port;  //takes 33uS to execute this post the falling edge
             
      } 

      INT0IF=0;
      
  }
  
  
  
  if (RBIF){
    
      
      
      RBIF=0;
  }
}
