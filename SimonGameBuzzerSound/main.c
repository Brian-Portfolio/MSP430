
/**
 * main.c
 */
#include <msp430.h>
#include <stdint.h>

#define TIMER_CFG_STOP TASSEL__ACLK | MC__STOP | TACLR //ACLK is used, stops debouncer timer, clear the bits
#define TIMER_CFG_UP   TASSEL__ACLK | MC__UP   | TACLR //ACLK is used, start debouncer timer, clear the bits

//tone frequency tone array
int tone[12] = { 157.80, 148.94, 132.69, 125.24, 111.58, 105.31, 99.408, 93.829,
                 83.59, 78.901, 74.472, 70.293 };

//winning melody tone array
int melody[32] = { 148.94, 125.24, 111.58, 111.58, 111.58, 99.408, 93.829,
                   93.829, 93.829, 83.59, 99.408, 99.408, 111.58, 125.24,
                   125.24, 111.58, 148.94, 125.24, 111.58, 111.58, 111.58,
                   93.829, 83.59, 83.59, 83.59, 74.472, 70.293, 70.293, 74.472,
                   83.59, 74.472, 111.58 };

//delay array for winning melody
unsigned int notes_delay[32] = { 12500, 12500, 25000, 25000, 12500, 12500,
                                 25000, 25000, 12500, 12500, 25000, 25000,
                                 12500, 12500, 12500, 50000, 12500, 12500,
                                 25000, 25000, 12500, 12500, 25000, 25000,
                                 12500, 12500, 25000, 25000, 12500, 12500,
                                 12500, 50000 };

//global variable turn on/off WIN state
int flag = 0;

//array used for LFSR
int array[10] = { };

//global button variables
unsigned int button1 = 99;

//MAX_LENGTH
int MAX_LENGTH = 6;

//LCD SCORE
#define ZERO 0xFC
#define ONE 0x60
#define TWO 0xDB
#define THREE 0xF3
#define FOUR 0x67
#define FIVE 0xB7
#define SIX 0xBF
#define SEVEN 0xE0
#define EIGHT 0xFF
#define NINE 0xE7

// Update the LFSR
uint16_t update_LFSR(uint16_t LFSR)
{

    uint16_t new_val;

    new_val = ((LFSR & BIT0)) ^  //create new bit to be rotated in
            ((LFSR & BIT1) >> 1) ^ ((LFSR & BIT3) >> 3) ^ ((LFSR & BIT5) >> 5);

    LFSR = LFSR >> 1;             //shift to perform rotation
    LFSR &= ~(BITF);             //have to clear bit because shift is arithmetic
    LFSR |= (new_val << 15);      //combine with new bit

    return LFSR;
}

void update_sequence(uint16_t LFSR)
{
    int i;
    i = 0;
    while (i < MAX_LENGTH)
    {
        LFSR = update_LFSR(LFSR);
        int seed = LFSR & (BIT1 | BIT0);
        array[i] = seed;
        i++;
    }

}

void onboard_seg_display_init(void)
{
    PJSEL0 = BIT4 | BIT5;                   // For LFXT

    LCDCPCTL0 = 0xFFD0;     // Init. LCD segments 4, 6-15
    LCDCPCTL1 = 0xF83F;     // Init. LCD segments 16-21, 27-31
    LCDCPCTL2 = 0x00F8;     // Init. LCD segments 35-39

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure LFXT 32kHz crystal
    CSCTL0_H = CSKEY >> 8;                  // Unlock CS registers
    CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
    do
    {
        CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
        SFRIFG1 &= ~OFIFG;
    }
    while (SFRIFG1 & OFIFG);               // Test oscillator fault flag
    CSCTL0_H = 0;                           // Lock CS registers

    // Initialize LCD_C
    // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;

    // VLCD generated internally,
    // V2-V4 generated internally, v5 to ground
    // Set VLCD voltage to 2.60v
    // Enable charge pump and select internal reference for it
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;

    LCDCCPCTL = LCDCPCLKSYNC;               // Clock synchronization enabled

    LCDCMEMCTL = LCDCLRM;                   // Clear LCD memory

    LCDCCTL0 |= LCDON;

}

void win_delay(volatile unsigned loops)
{
    while (--loops)
        ;

}
void delay(volatile unsigned long loops)
{
    while (--loops > 0)
        ; // Count down until the delay counter reaches 0

}

void msp_init()
{

    WDTCTL = WDTPW | WDTHOLD;     // Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;          // Unlock ports from power manager

    P1DIR |= BIT0;
    P1REN |= BIT1;
    P1OUT |= BIT1;

    P2DIR &= ~(BIT0 | BIT1 | BIT2 | BIT3);        // Set pin P2.0 to be an input
    P2REN |= (BIT0 | BIT1 | BIT2 | BIT3); // Enable internal pullup/pulldown resistor on P2.0
    P2OUT |= (BIT0 | BIT1 | BIT2 | BIT3);             // Pullup selected on P2.0

    P1IES |= BIT1;
    P1IFG &= ~BIT1;
    P1IE |= BIT1;

    P2IES |= BIT0;
    P2IFG &= ~BIT0;
    P2IE |= BIT0;

    P2IES |= BIT1;          // Make P2.0 interrupt happen on the falling edge
    P2IFG &= ~BIT1;               // Clear the P2.0 interrupt flag
    P2IE |= BIT1;                 // Enable P2.0 interrupt

    P2IES |= BIT2;
    P2IFG &= ~BIT2;
    P2IE |= BIT2;

    P2IES |= BIT3;
    P2IFG &= ~BIT3;
    P2IE |= BIT3;

    P3DIR |= (BIT0);             // Set pin 3.0 to be an output

    P3DIR |= (BIT1);             // Set pin 3.1 to be an output

    P3DIR |= (BIT2);             // Set pin 3.2 to be an output

    P3DIR |= (BIT3);             // Set pin 3.3 to be an output

    P2DIR |= 0xF0;              // make pins P2.{4-7} outputs
    P2SEL0 |= 0xF0;             // select module 1 of 3 (module 0 is GPIO)
    P2SEL1 &= 0x0F;             // ... see page 97 in the datasheet (slas789b)

    TA2CCR0 = 16384;            // 25 ms * 32768 Hz = 819.2 ticks debounce delay
    TA2CCTL0 = CCIE;        // Enable debounce timer interrupt
    TA2CTL = TASSEL__ACLK | MC__STOP | TACLR; // Configure debounce timer but don't start it

    // set output mode to reset/set (see page 459 in user's guide - slau367f)
    TB0CCTL3 = OUTMOD_7;    //TB0CCTL4 = TB0CCTL5 = TB0CCTL6 = OUTMOD_7;

    // clock source = ACLK divided by 1, put timer in UP mode, clear timer register
    TB0CTL = TBSSEL__ACLK | ID__1 | MC__UP | TBCLR;

    __enable_interrupt();      // Set global interrupt enable bit in SR register


}

#pragma vector = PORT1_VECTOR
__interrupt void p1_ISR()
{
    switch (P1IV)
    {
    case P1IV_NONE:
        break;
    case P1IV_P1IFG1:
        button1 = 4;
        P1OUT |= BIT0;
        delay(25000);
        P1OUT &= ~BIT0;
        __bic_SR_register_on_exit(LPM3_bits);
        break;
    }
}

#pragma vector = PORT2_VECTOR
__interrupt void p2_ISR()
{
    switch (P2IV)
    {
    case P2IV_NONE:
        break;
    case P2IV_P2IFG0:      // green button
        __bic_SR_register_on_exit(LPM3_bits);
        button1 = 0;
        P3OUT |= BIT0;
        TB0CCR0 = TB0CCR3 = tone[9];
        while (!(P2IN & BIT0))
            ;
        TB0CCR0 = TB0CCR3 = 0;
        P3OUT &= ~BIT0;
        break;
    case P2IV_P2IFG1:      // red button
        __bic_SR_register_on_exit(LPM3_bits);
        button1 = 1;
        P3OUT |= BIT1;
        TB0CCR0 = TB0CCR3 = tone[4];
        while (!(P2IN & BIT1))
            ;
        TB0CCR0 = TB0CCR3 = 0;
        P3OUT &= ~BIT1;
        break;
    case P2IV_P2IFG2:      // yellow button
        __bic_SR_register_on_exit(LPM3_bits);
        button1 = 2;
        P3OUT |= BIT2;
        TB0CCR0 = TB0CCR3 = tone[2];
        while (!(P2IN & BIT2))
            ;
        TB0CCR0 = TB0CCR3 = 0;
        P3OUT &= ~BIT2;
        break;
    case P2IV_P2IFG3:      // blue button
        __bic_SR_register_on_exit(LPM3_bits);
        button1 = 3;
        P3OUT |= BIT3;
        TB0CCR0 = TB0CCR3 = tone[0];
        while (!(P2IN & BIT3))
            ;
        TB0CCR0 = TB0CCR3 = 0;
        P3OUT &= ~BIT3;
        break;
    case P2IV_P2IFG4:
        break;
    case P2IV_P2IFG5:
        break;
    case P2IV_P2IFG6:
        break;
    case P2IV_P2IFG7:
        break;
    default:
        break;
    }
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void debounce_ISR()
{
    TA2CTL = TIMER_CFG_STOP; // stop debounce timer
    P2IE |= BIT0;                 // Re-enable interrupt after debouncing

    if (TA2CCR0 == 16384)
    {
        P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
        TA2CCR0 = 8192;
    }
    else {
        P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
        TA2CCR0 = 16384;
    }
    TA2CTL = TIMER_CFG_UP; //start debounce timer

}

void main(void)
{
    P3OUT &= ~BIT0;
    P3OUT &= ~BIT1;
    P3OUT &= ~BIT2;
    P3OUT &= ~BIT3;

    msp_init();
    onboard_seg_display_init();     // Init the LCD

    uint16_t LFSR = 0xABCD;
    LFSR = update_LFSR(LFSR);
    update_sequence(LFSR);

    //int array[] = { 1, 2, 3, 0, 2, 3, 2, 3, 2, 3 };

    int sequence[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    int score[] = { ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE };

    //initialize variables
    int j = 0;
    int current_level = 0;
    int i;
    int k;
    int blink1 = 2;
    int win = 1;
    int blink = 3;
    int fail = 1;

    //initialize states
    enum states
    {
        START, DISPLAY, CHECK, FAIL, WIN
    };
    enum states current;
    current = START;

    while (1)
    {
        switch (current)
        {
        case START:
            __bis_SR_register(LPM3_bits);
            if (button1 == 4)
            {
                j = 0;
                i = j;
                fail = 1;
                win = 1;
                sequence[i] = array[i];    // copy index of array onto sequence
                LCDM8 = score[0];
                current_level = 0;

                if (sequence[i] == 0) //GREEN LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[9];
                    P3OUT |= BIT0;
                    delay(50000);
                    P3OUT &= ~BIT0;
                    TB0CCR0 = TB0CCR3 = 0;
                    while (current_level == 0)
                    {
                        if (button1 == 0)
                        {

                            button1 = 99;
                            j++;
                            current_level++;
                            current = DISPLAY;
                        }
                        else if (button1 == 1 | button1 == 2 | button1 == 3)
                        {
                            current_level++;
                            current = FAIL;

                        }

                    }


                }

                else if (sequence[i] == 3) //BLUE LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[0];
                    P3OUT |= BIT3;
                    delay(50000);
                    P3OUT &= ~BIT3;
                    TB0CCR0 = TB0CCR3 = 0;
                    while (current_level == 0)
                    {
                        if (button1 == 3)
                        {

                            button1 = 99;
                            j++;
                            current_level++;
                            current = DISPLAY;

                        }
                        else if (button1 == 0 | button1 == 1 | button1 == 2)
                        {

                            current_level++;
                            current = FAIL;

                        }

                    }


                }
                else if (sequence[i] == 2) //YELLOW LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[2];
                    P3OUT |= BIT2;
                    delay(50000);
                    P3OUT &= ~BIT2;
                    TB0CCR0 = TB0CCR3 = 0;
                    while (current_level == 0)
                    {
                        if (button1 == 2)
                        {
                            button1 = 99;
                            j++;
                            current_level++;
                            current = DISPLAY;
                        }
                        else if (button1 == 0 | button1 == 1 | button1 == 3)
                        {
                            current_level++;
                            current = FAIL;

                        }

                    }

                }

                if (sequence[i] == 1) // RED LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[4];
                    P3OUT |= BIT1;
                    delay(50000);
                    P3OUT &= ~BIT1;
                    TB0CCR0 = TB0CCR3 = 0;
                    while (current_level == 0)
                    {
                        if (button1 == 1)
                        {
                            button1 = 99;
                            j++;
                            current_level++;
                            current = DISPLAY;
                        }
                        else if (button1 == 0 | button1 == 2 | button1 == 3)
                        {
                            current_level++;
                            current = FAIL;

                        }


                    }


                }


            }

            break;

        case DISPLAY:
            i = j;    // keep track of index value
            sequence[i] = array[i];    // copy index of array onto sequence
            i = 0;
            j++;
            LCDM8 = score[current_level];
            current_level++;
            while (i < current_level)
            {

                if (sequence[i] == 0) //GREEN LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[9];
                    P3OUT |= BIT0;
                    delay(50000);
                    P3OUT &= ~BIT0;
                    TB0CCR0 = TB0CCR3 = 0;
                }
                else if (sequence[i] == 3) //BLUE LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[0];
                    P3OUT |= BIT3;
                    delay(50000);
                    P3OUT &= ~BIT3;
                    TB0CCR0 = TB0CCR3 = 0;
                }
                else if (sequence[i] == 2) //YELLOW LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[2];
                    P3OUT |= BIT2;
                    delay(50000);
                    P3OUT &= ~BIT2;
                    TB0CCR0 = TB0CCR3 = 0;
                }
                else if (sequence[i] == 1) // RED LED turns on/off
                {
                    TB0CCR0 = TB0CCR3 = tone[4];
                    P3OUT |= BIT1;
                    delay(50000);
                    P3OUT &= ~BIT1;
                    TB0CCR0 = TB0CCR3 = 0;
                }
                i++;

                if (i == current_level)
                {
                    current = CHECK;
                }
            }
            break;

        case CHECK:
            i = 0;
            __bis_SR_register(LPM3_bits);
            while (i < current_level && i != 99)
            {

                if (button1 == 0)
                {
                    if (button1 == sequence[i])
                    {

                        button1 = 99;
                        i++;
                    }
                    else
                    {
                        i = 99;
                    }

                }

                else if (button1 == 1)
                {
                    if (button1 == sequence[i])
                    {

                        button1 = 99;
                        i++;
                    }
                    else
                    {
                        i = 99;
                    }

                }

                else if (button1 == 2)
                {
                    if (button1 == sequence[i])
                    {

                        button1 = 99;
                        i++;
                    }
                    else
                    {
                        i = 99;
                    }

                }

                else if (button1 == 3)
                {
                    if (button1 == sequence[i])
                    {

                        button1 = 99;
                        i++;

                    }
                    else
                    {
                        i = 99;
                    }

                }


            }

            if (current_level == MAX_LENGTH && i != 99)
            {
                current = WIN;
            }
            else if (i == 99)
            {
                current = FAIL;
            }
            else
                current = DISPLAY;
            break;

        case FAIL:

            while (fail < blink1)
            {

                TB0CCR0 = TB0CCR3 = 399.60; //82 Hz tone tick value
                P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
                delay(150000);
                P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
                TB0CCR0 = TB0CCR3 = 0;
                button1 = 99;
                fail++;
            }
            current = START;
            break;

        case WIN:
            k = 0;
            TA2CTL = TIMER_CFG_UP;
            TA2CCTL0 = CCIE;        // Enable debounce timer interrupt
            P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
            while (k <= 31)
            {
                flag = 0;
                TB0CCR0 = TB0CCR3 = melody[k];
                win_delay(notes_delay[k]);
                k++;
            }
            P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
            flag = 1;
            TA2CCTL0 = 0;                 // Disable debounce timer interrupt
            TA2CTL = TIMER_CFG_STOP;      // Stop debounce timer
            TB0CCR0 = TB0CCR3 = 0;
            current = START;
            break;
        }

    }
}

