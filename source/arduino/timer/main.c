////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17

Ex: A simple program that uses PB5 (Pin13 - led)
and PD2 (Pin2 - interrupt INT0), and a timer.

This builds on interrupt1 with adding a timer into
the routine.  The purpose is to add a systick counter
with a 1ms timebase.  This will be used for the delay
function so we can get rid of the libs.

Defines (see makefile)  __AVR_ATmega328P__
Inludes:  /usr/lib/avr/include

////////////////////////////////////////////
update 1/17/19 - Add Timer 2 configured at 1000hz timeout
with interrupt.  Similar setup as Timer0, Timer2 is an 8 bit 
timer/counter, configurable with overflow interrupt.  

Use the following registers - similar to Timer0
TCNT2 - Timer/counter
OCR2A - output compare register
OCR2B - output compare register
TIFR2 - Timer interrupt flag register
TIMSK2 - Timer interrupt mask register


TCCR2A - Timer/Counter control register
        - leave this one alone

TCCRB - Timer Control reg B
        - update bits 0-2 only - clock
        - probably the same as timer0

TCNT2 - Timer/Counter register -
        may need to update this one to 
        fine tune freq.

OCR2A / OCR2B - output compare - leave this alone

TIMSK2 - Interrupt mask register - 
        write 0x01 - enable the overflow
        interrupt

TIFR2 - Timer/Counter2 - interrupt flag
        Bit 0 - set when overflow interrupt
        occurs, cleared by hardware when executing the interrupt.
 



        









*/
///////////////////////////////////////////////

#include <avr/io.h>         //macros
#include <avr/interrupt.h>


//////////////////////////////////////////
//register defines
#define PORTB_DATA_R    (*((volatile unsigned char*)0x25))
#define PORTB_DIR_R     (*((volatile unsigned char*)0x24))
#define PINB_R          (*((volatile unsigned char*)0x23))

#define PORTD_DATA_R    (*((volatile unsigned char*)0x2B))
#define PORTD_DIR_R     (*((volatile unsigned char*)0x2A))
#define PIND_R          (*((volatile unsigned char*)0x29))

#define EICRA_R         (*((volatile unsigned char*)0x69))
#define EIMSK_R         (*((volatile unsigned char*)0x3D))
#define EIFR_R          (*((volatile unsigned char*)0x3C))
#define SREG_R          (*((volatile unsigned char*)0x5F))

#define PRR_R         (*((volatile unsigned char*)0x64))

//timer 0
#define TCCR0A_R         (*((volatile unsigned char*)0x44))
#define TCCR0B_R         (*((volatile unsigned char*)0x45))
#define TIMSK0_R         (*((volatile unsigned char*)0x6E))
#define TCNT0_R          (*((volatile unsigned char*)0x46))
#define TIFR0_R          (*((volatile unsigned char*)0x15))

//Timer2
#define TCCR2A_R         (*((volatile unsigned char*)0xB0))
#define TCCR2B_R         (*((volatile unsigned char*)0xB1))
#define TIMSK2_R         (*((volatile unsigned char*)0x70))
#define TCNT2_R          (*((volatile unsigned char*)0xB2))
#define TIFR2_R          (*((volatile unsigned char*)0x37))




//////////////////////////////////////
//prototypes
void GPIO_init(void);
void Interrupt_init(void);
void Timer0_init(void);
void Timer2_init(void);
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);

void Flash0(void);
void Flash1(void);
void Flash2(void);
void Flash3(void);
unsigned char flashRoutine = 0x00;     //0 - 3

/////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;



///////////////////////////////////////////
//ISR - Button
//
//waste some CPU time, check the button.
//if pressed, update the flash routine.
//The interrupt flag is self clearing by 
//calling this function.
//
ISR(INT0_vect)
{
    unsigned char val = 0x00;
    Waste_CPU(10000);

    //check button press after killing cpu time
    val = PIND_R & 0x04;
    if (!val)
    {
        if (flashRoutine < 3)
            flashRoutine++;
        else
            flashRoutine = 0;
    }
}


///////////////////////////////
//Timer0 Overflow Interrupt ISR
//Configured to run at 1khz, it's
//pretty close, runs a little slow.
//
ISR(TIMER0_OVF_vect)
{
    gTimeTick++;        //used by Delay
//    PINB_R |= 0x01;     //toggle PB0 (Pin 8)

    //clear interrupt - datasheet shows
    //this bit has to be set to run timer
    TIFR0 |= 0x01;
}



////////////////////////////////////
//Timer2 Overflow interrupt ISR
//Configured to run at 1khz - should
//be the same as Timer0 speed
ISR(TIMER2_OVF_vect)
{
    PINB_R |= 0x01;     //toggle PB0 (Pin 8)
    TIFR2 |= 0x01;      //clear the interrupt flag    
}


///////////////////////////////////////
int main()
{
    GPIO_init();        //configure led and button
    Interrupt_init();   //falling edge trigger  
    Timer0_init();      //Timer0 Counter Overflow
    Timer2_init();      //Timer2 Counter Overflow
 
    while(1)
    {
        switch(flashRoutine)
        {
            case 0: Flash0();   break;
            case 1: Flash1();   break;
            case 2: Flash2();   break;
            case 3: Flash3();   break;
        }
    }

	return 0;
}



///////////////////////////////////////////
//GPIO_init
//Configure led and button.  Note, according
//to datasheet, pullup can source current,
//so might be a good idea to add series 
//resistor.  either way works and not blew
//up anything yet.
//
void GPIO_init(void)
{    
   //Pin 13 - PB5
   PORTB_DIR_R |= 1u << 5; //pin 5 as output
   PORTB_DATA_R &=~ 1u << 5;   //clear

   //pin8
   PORTB_DIR_R |= 1u << 0; //pin 0 as output
   PORTB_DATA_R &=~ 1u << 0;   //clear

   //Pin 2 as input - PD2
   PORTD_DIR_R &=~ 1u << 2; //pin 2 as input
   PORTD_DATA_R &=~ 1u << 2; //clear for no pull
}


/////////////////////////////////
//Button PD2, falling edge trigger
//
void Interrupt_init(void)
{

    //configure INT0 - EICRA 0x02 falling edge
    EICRA_R |= 1u << 1;
    EICRA_R &=~ 0x01;

    //interrupt mask - EIMSK - 0x3D - set bit 0
    EIMSK_R |= 0X01;

    //clear pending interrupt - EIFR - 0X3C - bit 0
    EIFR_R |= 0x01;

    //enable global interrupts
    sei();
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
    TCCR0A = 0x00;

    //TCCR0B - Control = 0x45 - bits 0-2 confiure the prescaler.  dont care for remaining
    TCCR0B = 0x03;
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
    TIFR0 |= 0x01;

    //enable global interrupts
    sei();

}




//////////////////////////////////////////
//Configure Timer2 with Overflow Interrupt
//The configuration should be similar to Timer0
//in that both are 8 bit and will also be 
//configured to overflow at 1khz
//
void Timer2_init(void)
{
    //Timer Control - 2 registers:
    TCCR2A_R = 0x00;      //disable all pin outputs on compare
    TCCR2B_R = 0x04;      //set the prescaler - bits 0-2

    //NOTE: Prescale values for Timer2 are not the same
    //as Timer0.  ie, to get clk / 64, need to write 0x04

    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/32
    //    100 - clk/64
    //    101 - clk/128
    //    110 - clk/256
    //    111 - clk/1024

    TIMSK2_R |= 0x01;       //bit 0 - overflow interrupt
    TIFR2_R |= 0x01;        //clear the overflow flag

    sei();                  //enable interrupts
}






/////////////////////////////////////
void Waste_CPU(unsigned int temp)
{
    volatile unsigned int val = temp;
    while (val > 0)
    {
        val--;
        Dummy_Function();
    }
}


/////////////////////////////////////
void Dummy_Function(void)
{

}

/////////////////////////////////////
//Flash 50ms
void Flash0(void)
{
    PORTB_DATA_R |= 1u << 5;
    Delay(500);
    PORTB_DATA_R &=~ 1u << 5;
    Delay(500);
}

//Flash 100ms
void Flash1(void)
{
    PORTB_DATA_R |= 1u << 5;
    Delay(100);
    PORTB_DATA_R &=~ 1u << 5;
    Delay(100);
}

//Flash 200ms
void Flash2(void)
{
    PORTB_DATA_R |= 1u << 5;
    Delay(200);
    PORTB_DATA_R &=~ 1u << 5;
    Delay(200);
}

//Flash 500ms
void Flash3(void)
{
    PORTB_DATA_R |= 1u << 5;
    Delay(500);
    PORTB_DATA_R &=~ 1u << 5;
    Delay(500);
}



////////////////////////////////////
//timeTick is increased in timer isr
void Delay(volatile unsigned int val)
{
    gTimeTick = 0x00;           //upcounter
    while (val > gTimeTick){};

}












