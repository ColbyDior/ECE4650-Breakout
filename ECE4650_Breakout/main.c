#include <msp430fr5994.h>
#include <stdint.h>
#include "graphics.h"

// Global Variables

Graphics_Context g_sContext;

volatile unsigned char updateGame = 0;

// Function Prototypes

void DrawBricks(void);
void RestoreBricks(int oldBallX, int oldBallY);

// Main Function

int main(void)
{
    unsigned int joystickValue;
    unsigned int paddleX = 54;
    unsigned int targetPaddleX;

    int ballX = 64;
    int ballY = 105;
    int nextBallX;
    int nextBallY;

    int velocityX = 1;
    int velocityY = -2;

    unsigned char launched = 0;
    unsigned char launchPressed;
    unsigned char previousLaunchPressed = 0;

    Graphics_Rectangle paddle;

    WDTCTL = WDTPW | WDTHOLD;

    PM5CTL0 &= ~LOCKLPM5;

    // Keep the Working LCD Clock Setup

    CSCTL0 = CSKEY;
    CSCTL3 &= ~(BIT4 | BIT5 | BIT6);
    CSCTL0_H = 0;

    // Initialize the LCD

    Crystalfontz128x128_Init();
    Initialize_Graphics(&g_sContext);

    Graphics_setBackgroundColor(
        &g_sContext, GRAPHICS_COLOR_BLACK);

    Graphics_clearDisplay(&g_sContext);

    // Configure Joystick X-axis on P1.2 / A2

    P1SEL0 |= BIT2;
    P1SEL1 |= BIT2;

    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 = ADC12RES_2;
    ADC12CTL3 = 0;
    ADC12MCTL0 = ADC12INCH_2;
    ADC12CTL0 |= ADC12ENC;

    // Configure BoosterPack Buttons
    // S1 = P4.3, S2 = P4.2

    P4SEL0 &= ~(BIT3 | BIT2);
    P4SEL1 &= ~(BIT3 | BIT2);

    P4DIR &= ~(BIT3 | BIT2);
    P4OUT |= BIT3 | BIT2;
    P4REN |= BIT3 | BIT2;

    // Draw the Initial Screen

    DrawBricks();

    paddle.xMin = paddleX;
    paddle.xMax = paddleX + 19;
    paddle.yMin = 110;
    paddle.yMax = 114;

    Graphics_setForegroundColor(
        &g_sContext, GRAPHICS_COLOR_WHITE);

    Graphics_fillRectangle(&g_sContext, &paddle);
    Graphics_fillCircle(&g_sContext, ballX, ballY, 2);

    // Configure Timer_A0
    // Keep the Settings From the Working Movement Test

    TA0CTL = MC__STOP | TACLR;
    TA0EX0 = 0;
    TA0CCR0 = 33332;
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL__SMCLK | ID__8 | MC__UP | TACLR;

    __enable_interrupt();

    while (1)
    {
        if (updateGame == 0)
        {
            continue;
        }

        updateGame = 0;

        // Read the Joystick

        ADC12CTL0 |= ADC12SC;

        while (ADC12CTL1 & ADC12BUSY)
        {
        }

        joystickValue = ADC12MEM0;

        // Erase the Previous Paddle and Ball

        Graphics_setForegroundColor(
            &g_sContext, GRAPHICS_COLOR_BLACK);

        Graphics_fillRectangle(&g_sContext, &paddle);
        Graphics_fillCircle(&g_sContext, ballX, ballY, 2);

        // Restore Only Bricks Touched by the Previous Ball

        RestoreBricks(ballX, ballY);

        // Convert the Joystick Reading to a Target Position
        // Measured Joystick Range Is Approximately 683-3149

        if (joystickValue <= 700)
        {
            targetPaddleX = 0;
        }
        else if (joystickValue >= 3130)
        {
            targetPaddleX = 108;
        }
        else
        {
            targetPaddleX =
                (unsigned int)(
                    ((uint32_t)(joystickValue - 700) * 108)
                    / 2430
                );
        }

        // Limit Paddle Movement to 4 Pixels Per Update

        if (targetPaddleX > paddleX)
        {
            if (targetPaddleX - paddleX > 4)
            {
                paddleX += 4;
            }
            else
            {
                paddleX = targetPaddleX;
            }
        }
        else if (targetPaddleX < paddleX)
        {
            if (paddleX - targetPaddleX > 4)
            {
                paddleX -= 4;
            }
            else
            {
                paddleX = targetPaddleX;
            }
        }

        paddle.xMin = paddleX;
        paddle.xMax = paddleX + 19;

        // Read the Launch Button

        launchPressed = 0;

        if (!(P4IN & BIT3))
        {
            launchPressed = 1;
        }

        // S2 Resets the Ball
        // S1 Launches Only on a New Press

        if (!(P4IN & BIT2))
        {
            launched = 0;
            velocityX = 1;
            velocityY = -2;
        }
        else if (launchPressed == 1 &&
                 previousLaunchPressed == 0 &&
                 launched == 0)
        {
            ballX = paddleX + 10;
            ballY = 105;

            velocityX = 1;
            velocityY = -2;
            launched = 1;
        }

        previousLaunchPressed = launchPressed;

        // Update the Ball

        if (launched == 1)
        {
            nextBallX = ballX + velocityX;
            nextBallY = ballY + velocityY;

            // Left and Right Wall Collisions

            if (nextBallX < 2)
            {
                nextBallX = 2;
                velocityX = -velocityX;
            }
            else if (nextBallX > 125)
            {
                nextBallX = 125;
                velocityX = -velocityX;
            }

            // Top Wall Collision

            if (nextBallY < 2)
            {
                nextBallY = 2;
                velocityY = -velocityY;
            }

            // Paddle Collision
            // Bounce Only While Moving Down Across the Paddle Top

            if (velocityY > 0 &&
                ballY + 2 < paddle.yMin &&
                nextBallY + 2 >= paddle.yMin &&
                nextBallX + 2 >= paddle.xMin &&
                nextBallX - 2 <= paddle.xMax)
            {
                nextBallY = paddle.yMin - 3;
                velocityY = -velocityY;
            }

            // Missing the Paddle Returns the Ball to Ready

            if (nextBallY + 2 >= 127)
            {
                launched = 0;
                velocityX = 1;
                velocityY = -2;
            }
            else
            {
                ballX = nextBallX;
                ballY = nextBallY;
            }
        }

        // Hold the Ball Above the Paddle Until Launch

        if (launched == 0)
        {
            ballX = paddleX + 10;
            ballY = 105;
        }

        // Draw the Updated Paddle and Ball

        Graphics_setForegroundColor(
            &g_sContext, GRAPHICS_COLOR_WHITE);

        Graphics_fillRectangle(&g_sContext, &paddle);
        Graphics_fillCircle(&g_sContext, ballX, ballY, 2);
    }
}

// Draw the Initial Bricks

void DrawBricks(void)
{
    unsigned int row;
    unsigned int column;

    Graphics_Rectangle brick;

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
}

// Restore Only Bricks Overlapping the Erased Ball

void RestoreBricks(int oldBallX, int oldBallY)
{
    unsigned int row;
    unsigned int column;

    Graphics_Rectangle brick;

    if (oldBallY + 2 < 10 || oldBallY - 2 > 42)
    {
        return;
    }

    for (row = 0; row < 3; row++)
    {
        for (column = 0; column < 5; column++)
        {
            brick.xMin = 5 + column * 24;
            brick.xMax = brick.xMin + 21;

            brick.yMin = 10 + row * 12;
            brick.yMax = brick.yMin + 8;

            if (oldBallX + 2 >= brick.xMin &&
                oldBallX - 2 <= brick.xMax &&
                oldBallY + 2 >= brick.yMin &&
                oldBallY - 2 <= brick.yMax)
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

                Graphics_fillRectangle(&g_sContext, &brick);
            }
        }
    }
}

// Timer_A0 Interrupt

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
{
    updateGame = 1;
}
