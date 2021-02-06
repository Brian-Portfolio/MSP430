#include<stdio.h>
#include<stdint.h>
#include "STACK.h"

#define maxsize 6

unsigned long numbers[maxsize] = { 99, 99, 99, 99, 99, 99 };
unsigned long stackarray[maxsize];
int topofstack = -1;

void display_init()
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
    // return 0;

}

void push(unsigned long num)
{
    if (topofstack >= maxsize)
    {
        //display "FULL" on LCD
        LCDM10 = 0x8E; //F
        LCDM6 = 0x7C; //U
        LCDM4 = 0x1C; //L
        LCDM19 = 0x1C; //L
        LCDM8 = 0x00; //
        LCDM15 = 0x00; //
        return;
    }
    else
    {
        topofstack++;
        if (topofstack == 0)
        {
            LCDM14 |= BIT7; //B6
        }
        else if (topofstack == 1)
        {
            //LCDM14 |= BIT7;
            LCDM18 |= BIT7;
        }
        else if (topofstack == 2)
        {
            LCDM14 |= ( BIT6);
            //LCDM18 |= BIT7;
        }
        else if (topofstack == 3)
        {
            //LCDM14 |= (BIT7 | BIT6);
            LCDM18 |= (BIT6);
        }
        else if (topofstack == 4)
        {
            LCDM14 |= (BIT5);
            //LCDM18 |= (BIT7 | BIT6);
        }
        else if (topofstack == 5)
        {
            //LCDM14 |= (BIT7 | BIT6 | BIT5);
            LCDM18 |= ( BIT5);
        }
        stackarray[topofstack] = num;
        return;
    }
}

int pop(void)
{
    //int num;
    if (topofstack < 0)
    {
        //display "EMPTY" on LCD
        LCDM10 = 0x9E; //E
        LCDM6 = 0x6C; //M
        LCDM7 = 0xA0; //M
        LCDM4 = 0xCF; //P
        LCDM19 = 0x80; //T
        LCDM20 = 0x50; //T
        LCDM16 = 0xB0; //Y
        LCDM15 = 0x00;
        LCDM11 = 0x00;
        LCDM8 = 0x00;
        return NULL;
    }
    else
    {
        topofstack--;
        if (topofstack == 5)
        {
//            LCDM14 |= (BIT7 | BIT6 | BIT5);
//            LCDM18 |= (BIT7 | BIT6);
            LCDM18 &= ~BIT5;
        }
        else if (topofstack == 4)
        {
//            LCDM14 |= (BIT7 | BIT6);
//            LCDM18 |= (BIT7 | BIT6);
           // LCDM18 &= ~BIT5;
            LCDM14 &= ~BIT5;
        }
        else if (topofstack == 3)
        {
//            LCDM14 |= (BIT7 | BIT6);
//            LCDM18 |= (BIT7);
            LCDM18 &= ~( BIT6);
            //LCDM14 &= ~BIT5;
        }
        else if (topofstack == 2)
        {
//            LCDM14 |= (BIT7);
//            LCDM18 |= (BIT7);
            //LCDM18 &= ~(BIT5 | BIT6);
            LCDM14 &= ~( BIT6);
        }
        else if (topofstack == 1)
        {
//            LCDM14 |= (BIT7);
            LCDM18 &= ~( BIT7);
            //LCDM14 &= ~(BIT5 | BIT6);
        }
        else if (topofstack == 0)
        {
            //LCDM18 &= ~(BIT5 | BIT6 | BIT7);
            LCDM14 &= ~( BIT7);
        }
        return stackarray[topofstack + 1];
    }
}

