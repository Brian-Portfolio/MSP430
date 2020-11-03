#include <msp430.h>

void turnSegon(int segNum)
{

    if (segNum == 0)
    {
        LCDM10 |= 0x80;
    }

    if (segNum == 1)
    {
        LCDM6 |= 0x80;
    }

    if (segNum == 2)
    {
        LCDM4 |= 0x80;
    }

    if (segNum == 3)
    {
        LCDM19 |= 0x80;
    }

    if (segNum == 4)
    {
        LCDM15 |= 0x80;
    }

    if (segNum == 5)
    {
        LCDM8 |= 0x80;
    }

    if (segNum == 6)
    {
        LCDM8 |= 0x40;
    }

    if (segNum == 7)
    {
        LCDM8 |= 0x20;
    }

    if (segNum == 8)
    {
        LCDM8 |= 0x10;
    }

    if (segNum == 9)
    {
        LCDM15 |= 0x10;
    }

    if (segNum == 10)
    {
        LCDM19 |= 0x10;
    }

    if (segNum == 11)
    {
        LCDM4 |= 0x10;
    }

    if (segNum == 12)
    {
        LCDM6 |= 0x10;
    }

    if (segNum == 13)
    {
        LCDM10 |= 0x10;
    }

    if (segNum == 14)
    {
        LCDM10 |= 0x08;
    }

    if (segNum == 15)
    {
        LCDM10 |= 0x04;
    }
}

void turnSegoff(int num)
{

    if (num == 0)
    {
        LCDM10 &= ~0x80;
    }

    if (num == 1)
    {
        LCDM6 &= ~0x80;
    }

    if (num == 2)
    {
        LCDM4 &= ~0x80;
    }

    if (num == 3)
    {
        LCDM19 &= ~0x80;
    }

    if (num == 4)
    {
        LCDM15 &= ~0x80;
    }

    if (num == 5)
    {
        LCDM8 &= ~0x80;
    }

    if (num == 6)
    {
        LCDM8 &= ~0x40;
    }

    if (num == 7)
    {
        LCDM8 &= ~0x20;
    }

    if (num == 8)
    {
        LCDM8 &= ~0x10;
    }

    if (num == 9)
    {
        LCDM15 &= ~0x10;
    }

    if (num == 10)
    {
        LCDM19 &= ~0x10;
    }

    if (num == 11)
    {
        LCDM4 &= ~0x10;
    }

    if (num == 12)
    {
        LCDM6 &= ~0x10;
    }

    if (num == 13)
    {
        LCDM10 &= ~0x10;
    }

    if (num == 14)
    {
        LCDM10 &= ~0x08;
    }

    if (num == 15)
    {
        LCDM10 &= ~0x04;
    }
}

void delay(volatile unsigned int loops)
{
    if (loops > 90000)
    { // Max limit
        loops = 90000;
    }
    if (loops < 10000)
    { //Min limit
        loops = 10000;
    }

    while (--loops > 0)
        ; // Count down until the delay counter reaches 0
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

void main(void)
{

    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer

    onboard_seg_display_init();     // Init the LCD

    // init. external buttons
    P1DIR &= ~BIT0;

    P1REN |= BIT1; //button P1.1
    P1OUT |= BIT1; //change direction

    P1REN |= BIT2; //button P1.2
    P1OUT |= BIT2; //testing

    P2REN |= BIT0; //button P2.0
    P2OUT |= BIT0; //increase pace

    P2REN |= BIT1; //button P2.1
    P2OUT |= BIT1; //decrease pace

    P2REN |= BIT2; //button P2.2
    P2OUT |= BIT2; //increase length

    P2REN |= BIT3; //button P2.3
    P2OUT |= BIT3; //decrease length

    int i = 0;
    int k = i;
    int sec = 50000;
    int flag = 0;

    while (1)                        // loop continuously
    {
        //direction-clockwise
        if (flag == 0)
        {

            turnSegon(k);
            turnSegon(i);
            delay(sec);
            turnSegoff(i);
            k++; //head
            i++;
            if (i == 16)
            {
                i = 0;
            }
            if (k == 16)
            {
                k = 0;
            }

            if (!(P1IN & BIT1))
            {
                flag = 1;
            }

        }
        //direction-counter-clockwise
        if (flag == 1)
        {
            turnSegon(k);
            delay(sec);
            turnSegoff(k);
            k--; //tail
            i--;
            if (i == -1)
            {
                i = 15;
            }
            if (k == -1)
            {
                k = 15;
            }
            if (!(P1IN & BIT1))
            {
                flag = 0;
            }
        }

        //rate of pace
        if (!(P2IN & BIT0))
        {
            sec = sec + 10000;
            if (sec == 100000)
            {
                sec = 100000;
            }
        }
        else if (!(P2IN & BIT1))
        {
            sec = sec - 10000;
            if (sec == 10000)
            {
                sec = 10000;
            }
        }

        //increment length by 1(clockwise)
        if (flag == 0)
        {
            if (!(P2IN & BIT2))
            {
                turnSegon(k);
                k++;
                if (k == 16)
                {
                    k = 0;
                }


            }
        }

        //increment length by 1(counter-clockwise)
        if (flag == 1)
        {
            if (!(P2IN & BIT2))
            {
                turnSegon(i);
                i--;
                if (i == -1)
                {
                    i = 15;
                }
            }
            turnSegon(i);
        }

        //decrement length by 1(clockwise)
        if (flag == 0)
        {
            if (!(P2IN & BIT3))
            {
                turnSegoff(k);
                k--;
                if (k == 16)
                {
                    k = 0;
                }
            }
        }

        //decrement length by 1(counter-clockwise)
        if (flag == 1)
        {
            if (!(P2IN & BIT3))
            {
                turnSegoff(i);
                i++;
                if (i == -1)
                {
                    i = 15;
                }
            }
        }
    }
}
