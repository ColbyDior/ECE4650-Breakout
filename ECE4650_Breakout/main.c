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
        // Movement will be added in the next step.
    }
}
