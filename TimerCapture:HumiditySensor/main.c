/*
 *TimerCapture/HumiditySensor
 *
 *Timing the output pulse length from a humidity sensor to determine humidity.
 *Timers, Interrupts, and the capture and compare module available on your the MSP430,
 *to interface with DHC22 Humidity Sensor(External sensor connected to MSP430).
 *Read the humidity from the DHC22 every 2 seconds
 *and display the reading as a two-digit percentage value (00 to 99) on the LCD of MSP430.
 *the two seond delay will be implemented using timers and interrupts, and the MSP must be
 *in low-power mode while waiting.
 *
 *
*/


#include <msp430.h>
#include "lcd.h"

unsigned int lastTime = 0;
unsigned int length = 0;
unsigned char bitIndex = 0;
unsigned int temp = 0;
unsigned int hum = 0;
unsigned char valid = 0;

unsigned int bits[42];

enum modes {
    HIGH,
    BE,
    GO,
    WAITING
};

enum dispToggle {
    TEMP,
    HUM
};

enum modes mode;
enum dispToggle toggle;

void main(void)
{
    WDTCTL   = WDTPW | WDTHOLD; // Stop watchdog timer

    // starting with P1.7 as an output with high value
    P1DIR   |=  BIT7;
    P1OUT   |=  BIT7;
    // connect P1.7 to TA0.CCI2A
    P1SEL1   |=  BIT7;
    P1SEL0   |=  BIT7;

    // debug LEDs
    P1DIR |= BIT0;
    P9DIR |= BIT7;

    PM5CTL0 &= ~LOCKLPM5;       // Unlock ports from power manager

    mode = WAITING;

    lcd_init();

    // 1 second interrupt interval
    TA2CCR0 = 32768;
    TA2CTL = MC__UP | ID__1 | TASSEL__ACLK | TACLR;
    TA2CCTL0 = CCIE;

    lcd_clear();

    unsigned int data[5] = {0, 0, 0, 0, 0};

    __enable_interrupt();
    //_bis_SR_register(GIE | LPM0_bits);

    while(1){                   // continuous loop
        __low_power_mode_3();   // going to sleep but keep ACLK running
        unsigned char i = 0;
        unsigned char index = 0;
        // clear data array between runs
        data[0] = data[1] = data[2] = data[3] = data[4] = 0;
        // starts from two since that's where the first bit is
        for (i = 2; i < 42; i++) {
            unsigned char thisBit = 0;
            unsigned int thisLength = bits[i];
            // range for 0 bit
            if (thisLength >= 70 && thisLength <= 85) {
                thisBit = 0;
            }
            // range for 1 bit
            else if (thisLength >= 116 && thisLength <= 150) {
                thisBit = 1;
            }
            // shift this set of eight bits left and add on newest bit
            data[index] = (data[index] << 1) | thisBit;
            // boundaries between sets of eight bits
            if (i == 9 || i == 17 || i == 25 || i == 33) {
                index++;
            }
        }
        // combine upper and lower bits of hum
        hum = (data[0] << 8) | data[1];
        // combine upper and lower bits of temp
        temp = (data[2] << 8) | data[3];
        unsigned int parity = data[4];
        // result for comparison with parity, mask it with FF to trim it to 8 bits
        unsigned int result = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
        // not a valid result if it doesn't match parity
        if (parity != result) {
            valid = 0;
        } else {
            valid = 1;
        }
    }

}

// Interrupt Service Routine for Timer A
#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
{
    switch(__even_in_range(TA0IV,14))
    {
        case  0: break;
        case  2: break;                         // CCR1
        case  4:                                // CCR2
            // number of ticks between falling edges
            length = TA0CCR2 - lastTime;
            lastTime = TA0CCR2;
            // storing the number of sticks in a buffer
            bits[bitIndex] = length;
            // last falling edge of data
            if (bitIndex == 41) {
                // set P1.7 to be an output
                P1DIR    |=   BIT7;
                // stop TA0, which was in capture mode
                TA0CTL = MC__STOP | ID__1 | TASSEL__SMCLK | TACLR;
                TA0CCTL2 = CM_2 | CCIS_0 | SCS | CAP;
                __low_power_mode_off_on_exit();
            } else {
                // increase index in buffer
                bitIndex++;
            }
            break;
        case  6: break;                          // CCR3
        case  8: break;                          // CCR4
        case 10: break;                          // reserved
        case 12: break;                          // reserved
        case 14: P9OUT ^= BIT7; break;
        default: break;
    }
}

// 1 ms second interrupt
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR()
{
    TA1CCTL0 = 0;
    TA1CTL = MC__STOP | ID__1 | TASSEL__SMCLK | TACLR;
    // start TA0 in capture mode
    TA0CTL = MC__CONTINUOUS | ID__1 | TASSEL__SMCLK | TACLR | TAIE;
    TA0CCTL2 = CM_2 | CCIS_0 | SCS | CAP | CCIE;
    // set P1.7 to be an input
    P1DIR    &= ~BIT7;
    lastTime = 0;
}

// 1 second interrupt
#pragma vector = TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void)
{
    if (toggle == TEMP) {
        // set P1.7 to be an output
        P1DIR    |=   BIT7;
        // stop TA0, which was in capture mode
        TA0CTL = MC__STOP | ID__1 | TASSEL__SMCLK | TACLR;
        TA0CCTL2 = CM_2 | CCIS_0 | SCS | CAP;
        bitIndex = 0;
        P1OUT &= ~BIT7;
        // change mode to be idle at high
        mode = BE;
        toggle = HUM;
        // hold high for 10 ms before beginning of sequence
        TA1CCR0 = 1000;
        TA1CCTL0 = CCIE;
        TA1CTL = MC__UP | ID__1 | TASSEL__SMCLK | TACLR;
    } else {
        toggle = TEMP;
    }

    if (valid) {
        // show humidity
        if (toggle == HUM) {
            //lcd_show_word(HU);
            //lcd_show_num(hum);
            displayNum(hum, 3);
        }
        // show temperature
        else {
            //lcd_show_word(TP);
            //lcd_show_num(temp);
            displayNum(temp, 3);
        }
    } else {
        // show error message since parity didn't match
        //lcd_show_word(ERROR);
    }
    P1OUT ^= BIT0;
}
