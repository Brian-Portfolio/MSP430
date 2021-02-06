/*
 *
 *MatrixKeypad-Decoding
 *
 *A 4x4 matrix keypad, there are four column pins, four row pins,
 *and thus eight pins altogether.Each row is connected by a push button
 *to every column and vice versa. There will be debouncing of buttons in order to
 *receive consistent input from the keypad. The use of interrupts and timers are efficient
 *on the Launchpad.
 *
 *CalculatorFunctions:
 *Addition
 *Subtraction
 *Multiplication
 *Division
 *Pop
 *Push
 *
 */



#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "lcd.h"
#include "STACK.h"

#define TIMER_CFG_STOP TASSEL__ACLK | MC__STOP | TACLR;
#define TIMER_CFG_UP   TASSEL__ACLK | MC__UP   | TACLR;

//int testarray[6] = {};
//extern stackarray

//Keys
#define NONE 0x0F
#define ONE 0xE0 //10
#define TWO 0xD0//20
#define THREE 0xB0//40
#define Addition 0x70//80

#define FOUR 0xE1//11
#define FIVE 0xD1//21
#define SIX 0xB1//41
#define Subtraction 0x71//81

#define SEVEN 0xE2//12
#define EIGHT 0xD2//22
#define NINE 0xB2//42
#define Multiplication 0x72//82

#define POP 0xE3//13
#define ZERO 0xD3//23
#define PUSH 0xB3//43
#define Division 0x73//83

int button = 99;
int bit = 99;
int counter = 0;
unsigned long num = 0;
unsigned long new_num = 0;
//int i = 0;

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

void msp_init()
{
    WDTCTL = WDTPW | WDTHOLD;     // Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;         // Unlock ports from power manager

    P2DIR &= ~(BIT1 | BIT2 | BIT3 | BIT4);    // Set pin P2.0 to be an input
    P2REN |= (BIT1 | BIT2 | BIT3 | BIT4); // Enable internal pullup/pulldown resistor on P2.0
    P2OUT |= (BIT1 | BIT2 | BIT3 | BIT4);         // Pullup selected on P2.0

    P2IES |= (BIT1 | BIT2 | BIT3 | BIT4); // Make P2.0 interrupt happen on the falling edge
    P2IFG &= ~(BIT1 | BIT2 | BIT3 | BIT4);  // Clear the P2.0 interrupt flag
    P2IE |= (BIT1 | BIT2 | BIT3 | BIT4);            // Enable P2.0 interrupt

    P8DIR |= (BIT4 | BIT5 | BIT6 | BIT7);
    P8OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);

    // Timers A2/3 are good for debouncing since they don't have external pins
    TA2CCR0 = 8192;             // 25 ms * 32768 Hz = 819.2 ticks debounce delay
    TA2CCTL0 = CCIE;
    TA2CTL = TASSEL__ACLK | MC__STOP | TACLR; // Configure debounce timer but don't start it

    __enable_interrupt();      // Set global interrupt enable bit in SR register
}

#pragma vector = PORT2_VECTOR
__interrupt void p2_ISR()
{
    switch (P2IV)
    {
    case P2IV_NONE:
        break;
    case P2IV_P2IFG0:
        P2IE &= ~BIT0;          // Disable interrupt while debouncing
        P2IES ^= BIT0;          // Wait for opposite action (other edge)
        TA2CCTL0 = CCIE;        // Enable debounce timer interrupt
        TA2CTL = TIMER_CFG_UP
        ;  // Start debounce timer
        break;
    case P2IV_P2IFG1:
        button = 0;
        bit = BIT1;
        __low_power_mode_off_on_exit();
        break;

    case P2IV_P2IFG2:
        button = 1;
        bit = BIT2;
        __low_power_mode_off_on_exit();
        break;

    case P2IV_P2IFG3:
        button = 2;
        bit = BIT3;
        __low_power_mode_off_on_exit();
        break;

    case P2IV_P2IFG4:
        button = 3;
        bit = BIT4;
        __low_power_mode_off_on_exit();
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
    TA2CCTL0 = 0;                 // Disable debounce timer interrupt
    TA2CTL = TIMER_CFG_STOP
    ;      // Stop debounce timer
    P2IE |= BIT0;                 // Re-enable interrupt after debouncing

    if (TA2CCR0 == 8192)
    {
        P8OUT &= ~(BIT4 | BIT5 | BIT6 | BIT7);
        TA2CCR0 = 8192;
    }
    else
    {
        P8OUT |= (BIT4 | BIT5 | BIT6 | BIT7);
        TA2CCR0 = 8192;
    }
    TA2CTL = TIMER_CFG_UP
    ;
    //P2IFG &= ~BIT0;               // Clear interrupt flag until next action
//    if (P2IES & BIT0)
//        return;     // Only blink on presses
//    P2OUT ^= BIT4;                // Blink LED
}

void main(void)
{
    msp_init();
    onboard_seg_display_init();

    unsigned long difference;
    unsigned long sum;
    unsigned long quotient;
    unsigned long product;
    unsigned long num1;
    unsigned long num2;

    enum states
    {
        START, DISPLAY
    };
    enum states current;
    current = START;

    LCDM18 |= BIT4;
    LCDM14 |= BIT4;

    while (1)                            // Enter low power mode
    {

        switch (current)
        {

        case START:
            LCDM16 &= ~0xB0;
            LCDM20 &= ~0x50;
            LCDM7 &= ~0xA0;
            P2IE &= ~BIT1;                //disables interrupt
            P8OUT = ONE;                //P8OUT is set to column pin
            if (!(P2IN & bit))
            {
                if (button == 0)
                {
                    if (counter < 1)
                    {
                        num = 1;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        current = DISPLAY;
                    }
                }
            }
            else
            {
                P8OUT = TWO;
                if (!(P2IN & bit))
                {

                    if (button == 0)
                    {
                        if (counter < 1)
                        {
                            num = 2;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            current = DISPLAY;
                        }
                    }
                }
                else
                {
                    P8OUT = THREE;
                    if (!(P2IN & bit))
                    {

                        if (button == 0)
                        {
                            if (counter < 1)
                            {
                                num = 3;
                                displayNum(num, 6);
                                counter++;
                                button = 99;
                                current = DISPLAY;
                            }
                        }
                    }
                }
            }
            ////////ROW2//////////
            P8OUT = FOUR;                //P8OUT is set to column pin
            if (!(P2IN & bit))
            {
                if (button == 1)
                {
                    if (counter < 1)
                    {
                        num = 4;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        current = DISPLAY;
                    }
                }
            }
            else
            {
                P8OUT = FIVE;
                if (!(P2IN & bit))
                {

                    if (button == 1)
                    {
                        if (counter < 1)
                        {
                            num = 5;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            current = DISPLAY;
                        }
                    }
                }
                else
                {
                    P8OUT = SIX;
                    if (!(P2IN & bit))
                    {

                        if (button == 1)
                        {
                            if (counter < 1)
                            {
                                num = 6;
                                displayNum(num, 6);
                                counter++;
                                button = 99;
                                current = DISPLAY;
                            }
                        }
                    }
                }
            }
            /////////ROW3//////////
            P8OUT = SEVEN;                //P8OUT is set to column pin
            if (!(P2IN & bit))
            {
                if (button == 2)
                {
                    if (counter < 1)
                    {
                        num = 7;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        current = DISPLAY;
                    }
                }
            }
            else
            {
                P8OUT = EIGHT;
                if (!(P2IN & bit))
                {

                    if (button == 2)
                    {
                        if (counter < 1)
                        {
                            num = 8;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            current = DISPLAY;
                        }
                    }
                }
                else
                {
                    P8OUT = NINE;
                    if (!(P2IN & bit))
                    {

                        if (button == 2)
                        {
                            if (counter < 1)
                            {
                                num = 9;
                                displayNum(num, 6);
                                counter++;
                                button = 99;
                                current = DISPLAY;
                            }
                        }
                    }
                }
            }
            ///////row4/////////
            P8OUT = ZERO;                //P8OUT is set to column pin
            if (!(P2IN & bit))
            {
                if (button == 3)
                {
                    if (counter < 1)
                    {
                        num = 0;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        current = DISPLAY;
                    }
                }
            }
            P8OUT &= ~0xF0; // After testing column pin condition, clear P8OUT to 1's
            P2IFG &= ~BIT1;                // Clear interrupt flag
            P2IE |= BIT1;
            __low_power_mode_3();
            break;

        case DISPLAY:
            LCDM16 &= ~0xB0;
            LCDM20 &= ~0x50;
            LCDM7 &= ~0xA0;
            P2IE &= ~BIT1;
            P8OUT = ONE;
            if (!(P2IN & bit))
            {

                if (button == 0)
                {
                    new_num = 1;
                    if (counter >= 1 && counter < 7)
                    {
                        num = num * 10;
                        num = num + new_num;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        if (counter == 7)
                        {
                            counter = 0;
                            num = 0;
                            current = START;
                        }
                    }

                }
            }

            else
            {
                P8OUT = TWO;
                if (!(P2IN & bit))
                {
                    if (button == 0)
                    {
                        new_num = 2;
                        if (counter >= 1 && counter < 7)
                        {
                            num = num * 10;
                            num = num + new_num;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            if (counter == 7)
                            {
                                counter = 0;
                                num = 0;
                                current = START;
                            }
                        }

                    }
                }
                else
                {
                    P8OUT = THREE;
                    if (!(P2IN & bit))
                    {

                        if (button == 0)
                        {
                            new_num = 3;
                            if (counter >= 1 && counter < 7)
                            {
                                num = num * 10;
                                num = num + new_num;
                                displayNum(num, 6);
                                counter++;
                                button = 99;
                                if (counter == 7)
                                {
                                    counter = 0;
                                    num = 0;
                                    current = START;
                                }
                            }

                        }
                    }
                    else
                    {
                        P8OUT = Addition;
                        if (!(P2IN & bit))
                        {
                            if (button == 0)
                            {
                                num1 = pop();
                                num2 = pop();
                                sum = num1 + num2;
                                displayNum(sum, 6);
                                push(sum);
                                counter = 0;
                                current = START;
                            }
                        }

                    }
                }
            }

            ///////////ROW2////////////
            P8OUT = FOUR;
            if (!(P2IN & bit))
            {

                if (button == 1)
                {
                    new_num = 4;
                    if (counter >= 1 && counter < 7)
                    {
                        num = num * 10;
                        num = num + new_num;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        if (counter == 7)
                        {
                            counter = 0;
                            num = 0;
                            current = START;
                        }
                    }

                }
            }
            else
            {
                P8OUT = FIVE;
                if (!(P2IN & bit))
                {

                    if (button == 1)
                    {
                        new_num = 5;
                        if (counter >= 1 && counter < 7)
                        {
                            num = num * 10;
                            num = num + new_num;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            if (counter == 7)
                            {
                                counter = 0;
                                num = 0;
                                current = START;
                            }
                        }

                    }
                }
                else
                {
                    P8OUT = SIX;
                    if (!(P2IN & bit))
                    {

                        if (button == 1)
                        {
                            new_num = 6;
                            if (counter >= 1 && counter < 7)
                            {
                                num = num * 10;
                                num = num + new_num;
                                displayNum(num, 6);
                                counter++;
                                button = 99;
                                if (counter == 7)
                                {
                                    counter = 0;
                                    num = 0;
                                    current = START;
                                }
                            }

                        }
                    }
                    else
                    {
                        P8OUT = Subtraction;
                        if (!(P2IN & bit))
                        {
                            if (button == 1)
                            {
                                num1 = pop();
                                num2 = pop();
                                difference = num1 - num2;
                                displayNum(difference, 6);
                                push(difference);
                                counter = 0;
                                current = START;
                            }
                        }
                    }
                }

            }
            ///////////ROW3////////
            P8OUT = SEVEN;
            if (!(P2IN & bit))
            {

                if (button == 2)
                {
                    new_num = 7;
                    if (counter >= 1 && counter < 7)
                    {
                        num = num * 10;
                        num = num + new_num;
                        displayNum(num, 6);
                        counter++;
                        button = 99;
                        if (counter == 7)
                        {
                            counter = 0;
                            num = 0;
                            current = START;
                        }
                    }

                }
            }
            else
            {
                P8OUT = EIGHT;
                if (!(P2IN & bit))
                {

                    if (button == 2)
                    {
                        new_num = 8;
                        if (counter >= 1 && counter < 7)
                        {
                            num = num * 10;
                            num = num + new_num;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            if (counter == 7)
                            {
                                counter = 0;
                                num = 0;
                                current = START;
                            }
                        }

                    }
                }
                else
                {
                    P8OUT = NINE;
                    if (!(P2IN & bit))
                    {

                        if (button == 2)
                        {
                            new_num = 9;
                            if (counter >= 1 && counter < 7)
                            {
                                num = num * 10;
                                num = num + new_num;
                                displayNum(num, 6);
                                counter++;
                                button = 99;
                                if (counter == 7)
                                {
                                    counter = 0;
                                    num = 0;
                                    current = START;
                                }
                            }

                        }
                    }
                    else
                    {
                        P8OUT = Multiplication;
                        if (!(P2IN & bit))
                        {
                            if (button == 2)
                            {
                                num1 = pop();
                                num2 = pop();
                                product = num1 * num2;
                                displayNum(product, 6);
                                push(product);
                                counter = 0;
                                current = START;
                            }
                        }
                    }
                }

            }
            ///////////ROW4/////////
            P8OUT = POP;
            if (!(P2IN & bit))
            {
                if (button == 3)
                {
                    pop();
                    button = 99;
                    num = 0;
                    counter = 0;
                    current = START;
                }
            }
            else
            {
                P8OUT = ZERO;
                if (!(P2IN & bit))
                {

                    if (button == 3)
                    {
                        new_num = 0;
                        if (counter >= 1 && counter < 7)
                        {
                            num = num * 10;
                            num = num + new_num;
                            displayNum(num, 6);
                            counter++;
                            button = 99;
                            if (counter == 7)
                            {
                                counter = 0;
                                num = 0;
                                current = START;
                            }
                        }

                    }
                }
                else
                {
                    P8OUT = PUSH;
                    if (!(P2IN & bit))
                    {

                        if (button == 3)
                        {
                            push(num);
                            button = 99;
                            num = 0;
                            counter = 0;
                            current = START;
                        }
                    }
                    else
                    {
                        P8OUT = Division;
                        if (!(P2IN & bit))
                        {
                            if (button == 2)
                            {
                                num1 = pop();
                                num2 = pop();
                                quotient = num1 / num2;
                                displayNum(quotient, 6);
                                push(quotient);
                                counter = 0;
                                current = START;
                            }
                        }
                    }
                }

            }

            P8OUT &= ~0xF0;
            P2IFG &= ~BIT1;
            P2IE |= BIT1;
            __low_power_mode_3();
            break;
        }
    }
}
//__bis_SR_register(LPM3_bits);   // ACLK stays on in LPM3

