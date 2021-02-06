/**
 * Simon-Game
 *
 * Using four color LEDs with four associated push buttons. This program will blink the LEDs in random sequences
 * and the user has to remember this sequence. After the the sequence is completed, the user must repeat that same
 * sequence using the corresponding buttons to the LEDs. After each correct sequence the player follows. A new LED blink
 * blink will be added to the sequence. Length of the sequence corresponds to the current level. The game starts at level one
 * with a sequence of length one.
 *
 */

#include <msp430.h>
#include <stdint.h>

//sequence[10] = {};

//global button variables
unsigned int button1=99;

//MAX_LENGTH
int MAX_LENGTH = 5;

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
//uint16_t update_LFSR(uint16_t LFSR)
//{
//
//  uint16_t new_val;
//
//  new_val  = ((LFSR & BIT0)) ^  //create new bit to be rotated in
//             ((LFSR & BIT1) >> 1) ^
//             ((LFSR & BIT3) >> 3) ^
//             ((LFSR & BIT5) >> 5);
//
//  LFSR = LFSR >> 1;             //shift to perform rotation
//  LFSR &= ~(BITF);              //have to clear bit because shift is arithmetic
//  LFSR |= (new_val << 15);      //combine with new bit
//
//  return LFSR;
//}
//
//void update_sequence(uint16_t LFSR){
//    unsigned int t, i;
//    t=0;
//    while(i<MAX_LENGTH){
//        LFSR = update_LFSR(LFSR);
//        unsigned int seed = LFSR & (BIT1|BIT0);
//        sequence[i] = seed;
//    }
//}


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

void delay(volatile unsigned long loops)
{
    while (--loops > 0)
        ; // Count down until the delay counter reaches 0
}

void msp_init()
{

    WDTCTL = WDTPW | WDTHOLD;     // Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;          // Unlock ports from power manager

    P1DIR &= ~BIT1;
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
    case P2IV_P2IFG0:
        button1 = 0;
        break;
    case P2IV_P2IFG1:
        button1 = 1;
        break;
    case P2IV_P2IFG2:
        button1 = 2;
        break;
    case P2IV_P2IFG3:
        button1 = 3;
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

void main(void)
{
    P3OUT &= ~BIT0;
    P3OUT &= ~BIT1;
    P3OUT &= ~BIT2;
    P3OUT &= ~BIT3;

    msp_init();
    onboard_seg_display_init();     // Init the LCD

//    uint16_t LFSR = 0x5A5A;
//    LFSR = update_LFSR(LFSR);



    int score[] = { ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE };
    int array[] = { 1, 2, 3, 0, 2, 3, 2, 3, 2, 3 };
    //int array[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    int sequence[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    //update_sequence(LFSR);
    int j = 0;
    int current_level = 0;
    int i;
    int blink1 = 2;
    int win = 1;
    int blink = 3;
    int fail = 1;
    int flag = 0;

    enum states
    {
        START, DISPLAY, CHECK, FAIL, WIN
    };
    enum states current;
    current = START;
    msp_init();

    while (1)
    {
        switch (current)
        {
        case START:
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
                    P3OUT |= BIT0;
                    delay(50000);
                    P3OUT &= ~BIT0;
                    while (current_level == 0)
                    {
                        if (button1 == 0)
                        {
                            button1 = 99;
                            //j++;
                            current_level++;
                            current = DISPLAY;
                        }
                        else if (button1 == 1 | button1 == 2 | button1 == 3)
                        {
                            button1 = 99;
                            current_level = 1;
                            j = 0;
                            while (fail < blink1)
                            {
                                P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
                                delay(150000);
                                P2OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
                                fail++;
                                current = START;
                            }

                        }
                    }
                }

                else if (sequence[i] == 3) //BLUE LED turns on/off
                {
                    P3OUT |= BIT3;
                    delay(50000);
                    P3OUT &= ~BIT3;
                    while (current_level == 0)
                    {
                        if (button1 == 3)
                        {
                            button1 = 99;
                            //j++;
                            current_level++;
                            current = DISPLAY;
                        }
                        else if (button1 == 0 | button1 == 1 | button1 == 2)
                        {
                            button1 = 99;
                            current_level = 0;
                            j = 0;
                            while (fail < blink1)
                            {
                                P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
                                delay(150000);
                                P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
                                fail++;
                                current = START;

                            }

                        }
                    }
                }
                else if (sequence[i] == 2) //YELLOW LED turns on/off
                {
                    P3OUT |= BIT2;
                    delay(50000);
                    P3OUT &= ~BIT2;
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
                            button1 = 99;
                            current_level = 1;
                            j = 0;
                            while (fail < blink1)
                            {
                                P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
                                delay(150000);
                                P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
                                fail++;
                                current = START;

                            }

                        }
                    }
                }

                 if (sequence[i] == 1) // RED LED turns on/off
                {
                    P3OUT |= BIT1;
                    delay(50000);
                    P3OUT &= ~BIT1;
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
                            button1 = 99;
                            current_level = 1;
                            j = 0;
                            while (fail < blink1)
                            {
                                P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
                                delay(150000);
                                P2OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
                                fail++;
                                current = START;
                            }

                        }

                    }

                }
            }
            break;

        case DISPLAY:

            i = j;    // keep track of index value
            sequence[i] = array[i];    // copy index of array onto sequence
            i = 0;
            LCDM8 = score[current_level];
            current_level++;
            while (i < current_level)
            {

                if (sequence[i] == 0) //GREEN LED turns on/off
                {
                    P3OUT |= BIT0;
                    delay(50000);
                    P3OUT &= ~BIT0;

                }
                else if (sequence[i] == 3) //BLUE LED turns on/off
                {
                    P3OUT |= BIT3;
                    delay(50000);
                    P3OUT &= ~BIT3;

                }
                else if (sequence[i] == 2) //YELLOW LED turns on/off
                {
                    P3OUT |= BIT2;
                    delay(50000);
                    P3OUT &= ~BIT2;
                }
                else if (sequence[i] == 1) // RED LED turns on/off
                {
                    P3OUT |= BIT1;
                    delay(50000);
                    P3OUT &= ~BIT1;
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
                        else if (button1 == 0 | button1 == 2 | button1 == 3)
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
                        else if (button1 == 0 | button1 == 1 | button1 == 3)
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
                        else if (button1 == 0 | button1 == 1 | button1 == 2)
                        {
                            i = 99;;
                        }
                    }

                }
                j++;
                if (i == 99){
                    current = FAIL;
                }


                if (current_level == MAX_LENGTH)
                {
                    current = WIN;
                }
                else
                    current = DISPLAY;
                break;

                case FAIL:
                    while (fail < blink1)
                    {
                        P3OUT |= (BIT0 | BIT1 | BIT2 | BIT3);
                        delay(150000);
                        P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3);
                        button1 = 99;
                        fail++;
                    }
                    current = START;
                    break;

                case WIN:
                while (win <= blink)
                {
                    P3OUT |= BIT0;
                    P3OUT |= BIT1;
                    P3OUT |= BIT2;
                    P3OUT |= BIT3;
                    delay(50000);
                    P3OUT &= ~BIT0;
                    P3OUT &= ~BIT1;
                    P3OUT &= ~BIT2;
                    P3OUT &= ~BIT3;
                    delay(50000);

                    win++;

                }
                current = START;
                break;
            }

        }
    }
