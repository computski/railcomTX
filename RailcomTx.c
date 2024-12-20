/*
 * File:   RailcomTx.c
 * Author: julian
 *
 * Created on 24 June 2024, 10:05 PM
 * 2024-10-10 re-write to cope with dual polarity RC flags
 * RB0 supports INT on X
 * RB1 is wired to Y (no IOC) but also to RB7 which does generate IOC
 * we now need to look for a negative edge on either, and if immediately after this
 * both RB0,1 are zero then we are in the RC cutout.
 * If we are NOT in the RC cutout then we can gate the LED for a change, because this is on RB4
 * and writing to it will clear any pending IOC
 * 
 * 2024-10-18 EDGES.  Note that INT on any edge followed by both-hi will detect either polarity
 * same is true for IOC
 * And in fact all we need is INT, because depending on the edge seen, we can discriminate polarity
 * 
 */

//go to Production... set configuration bits.. Generate Source Code to Output and paste here
//The 886 device does not have output latches, we need shadow registers

//Note: MCLR has to be off, else it floats and erratic behaviour results.
//LVP does not seem to work.  In theory RB3 is used but the device says it programs but then it won't run
//so stick to regular HV program mode.  Also Pickit4 does not seem to have a LV entry mode for this chip


// CONFIG1  RA6 is clkout

#pragma config FOSC = INTOSCCLK //INTOSCIO  // Oscillator Selection bits (INTRC oscillator; port I/O function on both RA6/OSC2/CLKO pin and RA7/OSC1/CLKI pin)
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
#include<stdbool.h>
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
volatile bool rcActive;
volatile bool IOClastState;  //




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


void main(void) {
    OSCCONbits.IRCF=0b111;  //8MHz
    ANSEL=0;
    PORTA=sPORTA.port;
    TRISB4=0;
    TRISB5=0;  //TX output
    TRISA0=0;  //debug output
    TRISA6=0;
    
    
  //Timer 0
    OPTION_REGbits.T0CS=0;
    PSA=0;  //1=PSA to WDT, 0 = PSA to TMR0 giving div2
    OPTION_REGbits.PS=0b000; //prescaler to generate 1uS
    TMR0IE=0; //do not enable T0 ints
   
//timer 1. Boot default is instruction clock, no prescale
    TMR1CS=0;
    TMR1ON=0;
    TMR1IE=0;
    PEIE=0; //peripheral ints


    
    
    //page 99 of PIC16F88 spec
    TXEN = 1;
    SYNC = 0;
    SPEN = 1;
    //baud rate 250kbps, this is just possible on a 8MHz internal clock
    //TX is idle high, which results in nil current in the current loop.  A zero bit is a current of 30mA
    BRGH=1;
    SPBRG=1;
  
    //WPU and IOC
    nRBPU=1;
    RBIE=0;  //2024-10-18 do not enable IOC
    
    //INT config look for falling edge
    INTEDG = 0;
    INT0IE=1;
    rcActive=false;    
    
    ei();
    //OPTION_REGbits.
    
    //need a timer triggered off detection of IOC, T0 will do as we will count max 60 clock cycles
    
         
    
    while (1){
    __delay_ms(500);
    sPORTB.LED =! sPORTB.LED;
    
    //port writing is handled in the INT routine
    //PORTB=sPORTB.port;
    NOP();
    
    //to detect 
    
    
    
    }
    
    
    
    
    return;
}


//8MHz with 0.5uS instr clock, count 256 is 128uS
//RBIE monitors PORTB<7:4> for a change, read the port to clear it. note that read-modify-write will clear it.

//disabled TMR1, and now TMR0 transition is 7.2mS but that's way slow compared to the
//expected 50uS.   if I don't reset TMR0, I see 32mS.  


void __interrupt() ISR(){
  if (TMR0IF){
       TMR0IF=0;
       return;
       //we will use a fixed delay and not bother with TMR0  
  }    
     
  
  if (TMR1IF){
           TMR1IF=0;
           return;
           //code below not used
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
  
  
  //hmm. the routines are slow, e.g. RA0=1 is not asserted until 37uS after the edge falls.
  //so we are better off using fixed __delay_ms() within the Ch1, 2 calls and do this from within the INT
  
  
  if (INT0IF){
      
#ifdef not_using_this
      if (INTEDG){
          //saw falling edge,         
           if ((PORTB & 0b11)==0){
               //was an RC cutout, leave INTEG=0 as a signal to TMR0 routine
               rcActive=true;
              sPORTA.RA0=1;
              PORTA=sPORTA.port;
             INT0IE=0; //stop looking for edges
             
             // TMR0=255-T_TS1+T_CS+10;  //queue up CH1 transmission, 10 adjusts for INT execution delay
             //TMR0IF=0;
             
             //TMR0IE=1;  //use TMR0 to trigger Ch1 transmission
           
            
             
             __delay_us(30);
             //transmit Ch1 stuff 80uS after the entry edge
          //sending 0xAC 0x95 is 19 in the symbol table
             TXEN=1;  //send transmission
              TXREG=0xAC;  //with this data
              
              //wait for transmission to complete
              while (TRMT==0){NOP();}
              TXEN=1;  //send transmission
              TXREG=0x95;  //with this data
              
              
              
                 //now exit
              sPORTA.RA0=0;
              PORTA=sPORTA.port;
             INT0IE=1; //start looking for edges
             
           }else{
           //not an RC cutout, can assert port B to write to LED
               PORTB = sPORTB.port;
          
          }
           
      }
      //clear int
      INT0IF=0;     
#endif
      
      //2024-10-18 new block. Bug we don't see both high after a rising edge.
      //we probably need to add a small delay, i.e. sample 20uS after we see an edge
    __delay_us(10);
      //hmm this seems to garble the transmissions and it does not help on reverse polarity
      
      
      //if both low, we are in an RC cutout
       if ((PORTB & 0b11)==0){
              rcActive=true;
              sPORTA.RA0=1;
              PORTA=sPORTA.port;
             INT0IE=0; //stop looking for edges
          
             //but was it a rising or a falling edge?
             
             if (INTEDG){
             //rising
              __delay_us(30);
             TXEN=1;  //send transmission
              TXREG=0xC3;  //with this data
              
              //wait for transmission to complete
              while (TRMT==0){NOP();}
              TXEN=1;  //send transmission
              TXREG=0x96;  //with this data
       
             }else{
                 //falling
                 __delay_us(30);
                TXEN=1;  //send transmission
                TXREG=0xAC;  //with this data
              
                //wait for transmission to complete
                while (TRMT==0){NOP();}
                TXEN=1;  //send transmission
                TXREG=0x95;  //with this data
                }
             
            //now exit transmission
              sPORTA.RA0=0;
              PORTA=sPORTA.port;
             INT0IE=1; //start looking for edges
             
           }else{
           //not an RC cutout, can assert port B to write to LED
               PORTB = sPORTB.port;
          }
      
      //in all cases, invert edge we seek, and clear int
      INTEDG=~INTEDG;
      INT0IF=0;   
      
  }
  
  
  //2024-10-8 we don't use TMR0 we look for a rising then falling edge and if, just after the falling edge
  //both RB0,1 are high, this indicates an RC cutout.  A powered-half-bit will not have both high.
  if (RBIF){
            
 
     if (RB7==0){
            #ifdef not_using_this  
           //saw falling edge, but is this a RC cutout?
            if ((PORTB & 0b11)==0){
               //was an RC cutout
              sPORTA.RA0=1;
              PORTA=sPORTA.port;
              RBIE=0;  //stop looking for edges
              rcActive=true;
              
             __delay_us(30);
             //transmit Ch1 stuff 80uS after the entry edge
          //sending 0xAC 0x95 is 19 in the symbol table
             TXEN=1;  //send transmission
              TXREG=0xAB;  //with this data
              
              //wait for transmission to complete
              while (TRMT==0){NOP();}
              TXEN=1;  //send transmission
              TXREG=0x96;  //with this data
              
              //now exit
              sPORTA.RA0=0;
              PORTA=sPORTA.port;
             RBIE=1; //start looking for edges
             
           }else{
            //not an RC cutout, assert port B
            PORTB=sPORTB.port;
            
          }
    #endif     
      } 

      RBIF=0;
  }
}
