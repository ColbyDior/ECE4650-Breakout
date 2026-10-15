#include <msp430fr5994.h>
#include <stdint.h>
#include <stdio.h>
#include "graphics.h"

// Global Variables

Graphics_Context g_sContext;

volatile unsigned int minutes = 0;
volatile unsigned int seconds = 0;
volatile unsigned int timerCount = 0;
volatile unsigned char updateDisplay = 1;
volatile unsigned char running = 0;

// Main Function

int main(void)
{
    char timeString[6];

    WDTCTL = WDTPW | WDTHOLD;

    PM5CTL0 &= ~LOCKLPM5;

    CSCTL0 = CSKEY;
    CSCTL3 &= ~(BIT4 | BIT5 | BIT6);
    CSCTL0_H = 0;

    // Configure S1 and S2

    P5SEL0 &= ~(BIT5 | BIT6);
    P5SEL1 &= ~(BIT5 | BIT6);

    P5DIR &= ~(BIT5 | BIT6);     /
    P5REN |= BIT5 | BIT6;
    P5OUT |= BIT5 | BIT6;

    Crystalfontz128x128_Init();
    Initialize_Graphics(&g_sContext);

    // Configure Timer_A0

    // SMCLK / 8, up mode, clear timer
    TA0CTL = TASSEL__SMCLK | ID__8 | MC__UP | TACLR;

    TA0CCR0 = 62499;

    TA0CCTL0 = CCIE;

    __enable_interrupt();

    while (1)
    {
        if (!(P5IN & BIT5))
        {
            __delay_cycles(160000);

            if (!(P5IN & BIT5))
            {
                // Switch between running and paused
                if (running == 0)
                {
                    running = 1;
                }
                else
                {
                    running = 0;
                }

                // Wait until the button is released
                while (!(P5IN & BIT5));

                __delay_cycles(160000);
            }
        }

        // Reset Button

        if (!(P5IN & BIT6))
        {
            __delay_cycles(160000);

            if (!(P5IN & BIT6))
            {
                // Stop and reset the stopwatch
                running = 0;
                timerCount = 0;
                seconds = 0;
                minutes = 0;

                updateDisplay = 1;

                while (!(P5IN & BIT6));

                __delay_cycles(160000);
            }
        }

        // Update the Display

        if (updateDisplay == 1)
        {
            updateDisplay = 0;

            sprintf(timeString, "%02u:%02u", minutes, seconds);

            Graphics_drawStringCentered(
                &g_sContext,
                (int8_t *)timeString,
                AUTO_STRING_LENGTH,
                64,
                64,
                OPAQUE_TEXT
            );
        }
    }
}

// Timer_A0 Interrupt

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
{
    if (running == 1)
    {
        timerCount++;

        if (timerCount == 16)
        {
            timerCount = 0;
            seconds++;

            if (seconds == 60)
            {
                seconds = 0;
                minutes++;
            }

            if (minutes == 100)
            {
                minutes = 0;
            }

            updateDisplay = 1;
        }
    }
}
