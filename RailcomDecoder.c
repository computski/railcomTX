/*
 * File:   RailcomDecoder.c
 * Author: julian
 *
 * Created on 18 November 2024, 3:30 PM
 * 
 * This is a continuous decode engine.  it expects to see two bytes 4/8 encoded on the serial RX line and if these parse correctly
 * it will output the payload on the serial TX line.
 * 
 * 
 */

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

#include <xc.h>
#include<pic16f88.h>
#include<stdbool.h>
#define _XTAL_FREQ    8000000
#define BUFFER_SIZE     16

const uint8_t decode[] = {
0b10101100,0b10101010,0b10101001,0b10100101,0b10100011,0b10100110,0b10011100,0b10011010,0b10011001,0b10010101,0b10010011,0b10010110,0b10001110,0b10001101,0b10001011,0b10110001,
0b10110010,0b10110100,0b10111000,0b01110100,0b01110010,0b01101100,0b01101010,0b01101001,0b01100101,0b01100011,0b01100110,0b01011100,0b01011010,0b01011001,0b01010101,0b01010011,
0b01010110,0b01001110,0b01001101,0b01001011,0b01000111,0b01110001,0b11101000,0b11100100,0b11100010,0b11010001,0b11001001,0b11000101,0b11011000,0b11010100,0b11010010,0b11001010,
0b11000110,0b11001100,0b01111000,0b00010111,0b00011011,0b00011101,0b00011110,0b00101110,0b00110110,0b00111010,0b00100111,0b00101011,0b00101101,0b00110101,0b00111001,0b00110011,
0b00001111,0b11110000,0b11100001 };
#define RC_NACK 0x40
#define RC_ACK 0x41
#define RC_BUSY 0x42

struct RC_MSG {
	uint8_t state;
	uint16_t locoAddr;
	bool    useLongAddr;
	uint8_t payload;
	bool	isValid;
	uint8_t b1;
	uint8_t b2;
}rc_msg;

enum RC_STATE
{
	RC_EXPECT_ID0,
	RC_EXPECT_BYTE,
	RC_SUCCESS,
	RC_TIMEOUT,
};

//circular buffer
uint8_t serialBuffer[BUFFER_SIZE];
uint8_t serialBufferIdx;

volatile uint8_t LEDtimeout;


//define functions
bool decodeRailcom(uint8_t *inByte, bool ignoreControlCodes);
bool SerialRead(uint8_t *data);
void SerialWrite(uint8_t data);
void SerialDump(void);

//not using portA
volatile union {
uint8_t                 port;
struct {
                unsigned             RB0  :1;              //Sense1            
                unsigned             RB1   :1;             //Sense2             
				unsigned			 RX  :1;
                unsigned			 PGM   :1;
                unsigned             LED  	:1;                        
                unsigned             TX    :1;               //TX to line          
                unsigned             PGC	:1;
                unsigned             PGD	:1;
                
                };
} sPORTB;


//TX is pin 11 (RB5) and feeds into R4
//RX is pin 8 and appears on H3 pin 1
//LED is active low on RB4

//I intend to make the LED come on if we have a valid byte and then timeout after 0.5s


void main(void) {
    OSCCONbits.IRCF=0b111;  //8MHz
     
    TRISB0=0;
    TRISB2=1;
    TRISB4=0;
    TRISB5=0;
    
     //Set up Serial page 99 of PIC16F88 spec, am not sure if it has a buffer
    TXEN = 1;
    SYNC = 0;
    SPEN = 1;
    //baud rate 250kbps, this is just possible on a 8MHz internal clock
    //TX is idle high, which results in nil current in the current loop.  A zero bit is a current of 30mA
    BRGH=1;
    SPBRG=1;
    
    //extra stuff related to receive section 11.2.2 page 106
    CREN=1; //enable continuous receive
    
    uint8_t byteNew;
     
    rc_msg.state = RC_EXPECT_ID0; 
    
    //END boot section
    
    
    //timer 1. Boot default is instruction clock, no prescaler.
    //i.e. will overflow 
    TMR1CS=0;
    T1CONbits.T1CKPS=0b11;  //div8, giving 3.8Hz overflow
    TMR1ON=1;
    TMR1IE=1;
    PEIE=1; //peripheral ints

    
   /*
    while (true){
        sPORTB.LED = ~sPORTB.LED;
        sPORTB.RB0= ~sPORTB.RB0;     
        PORTB=sPORTB.port;  
    //we see about 37 khz on the loop
        
    }
    */
    
    
    uint8_t temp;
    
    /*
    while(true){
        SerialWrite(test++);  
        sPORTB.LED=~sPORTB.LED;
        PORTB=sPORTB.port;
        //works, I see data sent sequentially at 250kbps
    }
    */
    
    /*
    while(true){
        sPORTB.RB0= ~sPORTB.RB0; 
         PORTB=sPORTB.port;
        if (SerialRead(&test)){
              SerialWrite(test++);  
             sPORTB.LED=~sPORTB.LED;
        }
    //this test also works, and we see valid data on both polarities
    }
    */
    
    
    //MAIN LOOP
    while(true){

        //LED is active low
        //sPORTB.LED = LEDtimeout==0?1:0;
        sPORTB.RB0= ~sPORTB.RB0;     //this is crazy, I am seeing 5.6mS or 177Hz on this pin. how? the main loop should be running much faster.
        PORTB=sPORTB.port;
         

        // read the incoming byte:
        if	(SerialRead(&byteNew)){
            //advance buffer posn
            if (serialBufferIdx++==BUFFER_SIZE) serialBufferIdx=0;
            serialBuffer[serialBufferIdx]=byteNew;
            
            //preserve this and prior value as raw input data
            //b2 is chronologically the newest
            rc_msg.b1 = rc_msg.b2;
            rc_msg.b2=byteNew;
            
            for (uint8_t n=0;n<2;n++){
            temp= n==0?rc_msg.b1:rc_msg.b2;
                
            //now process the last TWO bytes
            
                
                switch (rc_msg.state) {
                case RC_EXPECT_BYTE:
                    sPORTB.LED=~sPORTB.LED;
                   // rc_msg.b2 = byteNew;
                    if (decodeRailcom(&temp,true)) {
                        rc_msg.payload += (temp & 0b00111111);
                        //SerialWrite(rc_msg.payload);
                        SerialDump();
                        /*
                        JsonDocument out;
                        out["type"] = "railcom";
                        char buff[6];
                        out["b1"] = itoa(rc_msg.b1, buff, 16);
                        out["b2"] = itoa(rc_msg.b2, buff, 16);
                        out["payload"] = rc_msg.payload;
                        out["flag"] = DCCpacket.railcomCutoutActive;
                        nsDCCweb::sendJson(out);
                        */
                    }
                    rc_msg.state = RC_EXPECT_ID0; 
                    break;

                case RC_SUCCESS:
                break;


                default:
                    //rc_msg.b1 = byteNew;

                    if (decodeRailcom(&temp,true)) {
                        if ((temp & 0b00111100) == 0) {
                            //found ID0
                            rc_msg.payload = temp << 6;
                            rc_msg.state = RC_EXPECT_BYTE;
                            break;
                        }
                    }
                    rc_msg.state = RC_EXPECT_ID0;
                    break;

                }
            }//end loop2
        }    
    } //end prog loop
    return;
}


void __interrupt() ISR(){
  if (TMR1IF){
       TMR1IF=0;
       LEDtimeout-=LEDtimeout>0?1:0;
  }    
}

//ANCILLARY ROUTINES

bool decodeRailcom(uint8_t *inByte, bool ignoreControlCodes) {
	for (uint8_t i = 0; i <= RC_BUSY; i++) {
		if (*inByte == decode[i]) {
			//valid
			*inByte = i;
			return true;
		}
        if (ignoreControlCodes && (i>=0x3F)) return false;
	}
	//didn't find a match
	
	return false;

}




/*
uint8_t SerialAvailable(void){
    //look for serial data, if found, add to buffer and clear any errors
    
    if (RCIF){
        serialBuffer[serialBufferW++]=RCREG;
        if (serialBufferW>=16) serialBufferW=0;
        //check for errors
        if (FERR || OERR) {
            CREN=0;
            NOP();
            CREN=1;
        }          
    }
    //is there data in the buffer? if R = W the answer is no because any new data causes the W pointer to advance
    
    return 0;
    
}
*/


//combine serial available and read into a single operation
//returns true if new data
bool SerialRead(uint8_t *data){
if (RCIF){
        *data=RCREG;
        //check for errors
        if (FERR || OERR) {
            CREN=0;
            NOP();
            CREN=1;
        }          
        return true;
    }
return false;
}

void SerialWrite(uint8_t data){
    TXEN=1;  //send transmission
    TXREG=data;  //with this data
    //make this blocking, i.e. wait until transmission is sent
    while (TRMT==0){NOP();}
}

void SerialDump(void){
//found good data (2nd byte) at serialBufferIdx
    
    //oldest bytes
    for (uint8_t n=serialBufferIdx+1;n<BUFFER_SIZE;n++){
        TXEN=1;  //send transmission
    TXREG=serialBuffer[n];  //with this data
    //make this blocking, i.e. wait until transmission is sent
    while (TRMT==0){NOP();}        
    }

     for (uint8_t n=0; n<=serialBufferIdx;n++){
           TXEN=1;  //send transmission
    TXREG=serialBuffer[n];  //with this data
   while (TRMT==0){NOP();}   
     }   

    //and the data itself
            TXEN=1;  //send transmission
    TXREG=rc_msg.payload;
   while (TRMT==0){NOP();}   
 

}    
