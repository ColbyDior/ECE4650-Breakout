#include <msp430fr5994.h>
#include <stdint.h>
#include "graphics.h"

// Global Variables

Graphics_Context g_sContext;

// Main Function

int main(void)
{
    unsigned int row;
    unsigned int column;
    unsigned int joystickValue;
    unsigned int paddleX = 54;
    unsigned int newPaddleX;
    unsigned char launched = 0;
    unsigned char previousLaunched = 2;

    Graphics_Rectangle paddle;
    Graphics_Rectangle brick;

    WDTCTL = WDTPW | WDTHOLD;

    PM5CTL0 &= ~LOCKLPM5;

    CSCTL0 = CSKEY;
    CSCTL3 &= ~(BIT4 | BIT5 | BIT6);
    CSCTL0_H = 0;

    // Initialize the LCD

    Crystalfontz128x128_Init();
    Initialize_Graphics(&g_sContext);

    // Configure Joystick X-axis on P1.2 / A2

    P1SEL0 |= BIT2;
    P1SEL1 |= BIT2;

    // Configure BoosterPack Buttons
    // S1 = P4.3, S2 = P4.2

    P4SEL0 &= ~(BIT3 | BIT2);
    P4SEL1 &= ~(BIT3 | BIT2);

    P4DIR &= ~(BIT3 | BIT2);
    P4OUT |= BIT3 | BIT2;
    P4REN |= BIT3 | BIT2;

    // Configure ADC for a single 12-bit conversion

    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 = ADC12RES_2;
    ADC12CTL3 = 0;

    ADC12MCTL0 = ADC12INCH_2;

    ADC12CTL0 |= ADC12ENC;

    Graphics_setBackgroundColor(
        &g_sContext, GRAPHICS_COLOR_BLACK);

    Graphics_clearDisplay(&g_sContext);

    // Draw Three Rows of Bricks

    for (row = 0; row < 3; row++)
    {
        if (row == 0)
        {
            Graphics_setForegroundColor(
                &g_sContext, GRAPHICS_COLOR_RED);
        }
        else if (row == 1)
        {
            Graphics_setForegroundColor(
                &g_sContext, GRAPHICS_COLOR_GREEN);
        }
        else
        {
            Graphics_setForegroundColor(
                &g_sContext, GRAPHICS_COLOR_BLUE);
        }

        for (column = 0; column < 5; column++)
        {
            brick.xMin = 5 + column * 24;
            brick.xMax = brick.xMin + 21;

            brick.yMin = 10 + row * 12;
            brick.yMax = brick.yMin + 8;

            Graphics_fillRectangle(&g_sContext, &brick);
        }
    }

    // Draw the Paddle

    paddle.xMin = 54;
    paddle.xMax = 73;
    paddle.yMin = 110;
    paddle.yMax = 114;

    Graphics_setForegroundColor(
        &g_sContext, GRAPHICS_COLOR_WHITE);

    Graphics_fillRectangle(&g_sContext, &paddle);

    // Draw the Ball Above the Paddle

    Graphics_fillCircle(&g_sContext, 64, 105, 2);

    while (1)
    {
        // Check BoosterPack Buttons
        // Reset takes priority if both buttons are pressed.

        if (!(P4IN & BIT2))
        {
            launched = 0;
        }
        else if (!(P4IN & BIT3))
        {
            launched = 1;
        }

        // Update the Status Only When It Changes

        if (launched != previousLaunched)
        {
            Graphics_setForegroundColor(
                &g_sContext, GRAPHICS_COLOR_WHITE);

            if (launched == 1)
            {
                Graphics_drawStringCentered(
                    &g_sContext,
                    (int8_t *)"LAUNCH",
                    AUTO_STRING_LENGTH,
                    64,
                    80,
                    OPAQUE_TEXT
                );
            }
            else
            {
                Graphics_drawStringCentered(
                    &g_sContext,
                    (int8_t *)"READY ",
                    AUTO_STRING_LENGTH,
                    64,
                    80,
                    OPAQUE_TEXT
                );
            }

            previousLaunched = launched;
        }

        // Read the Joystick

        ADC12CTL0 |= ADC12SC;

        while (ADC12CTL1 & ADC12BUSY)
        {
            // Wait for the conversion to finish.
        }

        joystickValue = ADC12MEM0;

        // Convert ADC range 0-4095 to paddle position 0-108.
        // Use 32-bit multiplication to prevent overflow.

        newPaddleX =
            (unsigned int)(((uint32_t)joystickValue * 108) / 4095);

        if (newPaddleX != paddleX)
        {
            // Erase the Previous Paddle and Ball

            Graphics_setForegroundColor(
                &g_sContext, GRAPHICS_COLOR_BLACK);

            Graphics_fillRectangle(&g_sContext, &paddle);

            Graphics_fillCircle(
                &g_sContext, paddleX + 10, 105, 2);

            // Update the Paddle Position

            paddleX = newPaddleX;

            paddle.xMin = paddleX;
            paddle.xMax = paddleX + 19;

            // Draw the Paddle and Ball at the New Position

            Graphics_setForegroundColor(
                &g_sContext, GRAPHICS_COLOR_WHITE);

            Graphics_fillRectangle(&g_sContext, &paddle);

            Graphics_fillCircle(
                &g_sContext, paddleX + 10, 105, 2);
        }

        // Temporary pacing for this joystick test.
        // Timer-controlled updates will come next.

        __delay_cycles(10000);
    }
}
