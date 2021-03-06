////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
1/6/18
A simple program that initializes i2c peripheral
located on pins PC4 and PC5.  The i2c peripheral
is tested using the tsl2561 light sensor ic.
Basic read functions for reading the product code,
writing data to registers, and reading registers
are provided in i2c.c/.h.


*/
///////////////////////////////////////////////

#include <avr/interrupt.h>
#include <avr/io.h>         //macros
#include <stdio.h>
#include <string.h>

#include "register.h"
#include "i2c.h"
#include "si5351.h"

//////////////////////////////////////
//prototypes
void GPIO_init(void);
void Timer0_init(void);

/////////////////////////////
//Delay items
void Delay(unsigned long val);
volatile unsigned long gTimeTick = 0x00;


///////////////////////////////
//Timer0 Overflow Interrupt ISR
//Configured to run at 1khz, it's
//pretty close, runs a little slow.
//
ISR(TIMER0_OVF_vect)
{
    gTimeTick++;        //used by Delay

    //clear interrupt - datasheet shows
    //this bit has to be set to run timer
    TIFR0_R |= 0x01;
}


///////////////////////////////////////
int main()
{
    GPIO_init();        //configure led and button
    Timer0_init();      //Timer0 Counter Overflow
    i2c_init();
//    vfo_init();

//    vfo_SetChannel0Frequency(10000);
//    vfo_SetChannelState(VFO_CHANNEL_0, VFO_STATE_ENABLE);


    while(1)
    {
//    	uint8_t code = vfo_ProductCode();

    	//write AA to reg 16
    	vfo_writeReg(16, 0xAA);

		PORTB_DATA_R ^= BIT5;
		PORTB_DATA_R ^= BIT0;
        Delay(10);
    }

	return 0;
}



///////////////////////////////////////////
//GPIO_init
//Configure pin 13, PB5 as output
//
void GPIO_init(void)
{    
   //Pin 13 - PB5
   PORTB_DIR_R |= BIT5; 	//pin 13 as output
   PORTB_DATA_R &=~ BIT5;	//clear pin 13

   PORTB_DIR_R |= BIT0; 	//pin 8 as output
   PORTB_DATA_R &=~ BIT0;	//clear pin 8
}



//////////////////////////////////////////
//Configure Timer0 with Overflow Interrupt
//
void Timer0_init(void)
{

    //enable timer TC0 is enabled with writing 1 to PRR.PRTIM0 -> reg 0x64, bit 5 - set
    //PRR_R |= 1u << 5;
    //Note - don't do this...
    //i saw in the datasheet where this bit has to be set but it kills it
   

    //Timer Control - 2 registers:
    //TCCR0A - Control = 0x44 - write 0x00 to disable all pin outputs on compare.
    TCCR0A_R = 0x00;

    //TCCR0B - Control = 0x45 - bits 0-2 confiure the prescaler.  dont care for remaining
    TCCR0B_R = 0x03;
    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/64
    //    100 - clk/256
    //    101 - clk/1024

    //TIMSK0 - int mask reg = 0x6E - bit 0 is overflow interrupt => write 0x01
    TIMSK0_R |= 0x01;
    
    //clear interrupt
    //TIFR0 - interrupt flag.  0x15    TOV - write 1 to clear interrupt
    TIFR0_R |= 0x01;

    //enable global interrupts
    __SEI;
}



////////////////////////////////////
//timeTick is increased in timer isr
void Delay(unsigned long val)
{
	volatile unsigned long t = val;
    gTimeTick = 0x00;           //upcounter
    while (t > gTimeTick){};
}



