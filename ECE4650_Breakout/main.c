#include <msp430fr5994.h>
#include <stdint.h>
#include "graphics.h"

// Global Variables

Graphics_Context g_sContext;

volatile unsigned char updateGame = 0;

Graphics_Rectangle bricks[15];
unsigned char brickActive[15];
unsigned int bricksRemaining = 15;

// Function Prototypes

void ResetBoard(void);
void DrawBrickCount(void);

// Main Function

int main(void)
{
    unsigned int joystickValue;
    unsigned int paddleX = 54;
    unsigned int targetPaddleX;
    unsigned int i;

    int ballX = 64;
    int ballY = 105;
    int nextBallX;
    int nextBallY;

    int velocityX = 1;
    int velocityY = -2;

    unsigned char launched = 0;
    unsigned char gameWon = 0;

    unsigned char launchPressed;
    unsigned char resetPressed;
    unsigned char previousLaunchPressed = 0;
    unsigned char previousResetPressed = 0;

    unsigned char hitHorizontal;
    unsigned char hitVertical;

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

    ResetBoard();

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

        // Convert the Joystick Reading to a Target Position
        // Calibrated for the Measured Joystick Range

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

        // Read the BoosterPack Buttons

        launchPressed = 0;
        resetPressed = 0;

        if (!(P4IN & BIT3))
        {
            launchPressed = 1;
        }

        if (!(P4IN & BIT2))
        {
            resetPressed = 1;
        }

        // Reset Takes Priority Over Launch

        if (resetPressed == 1)
        {
            if (previousResetPressed == 0)
            {
                launched = 0;
                gameWon = 0;

                velocityX = 1;
                velocityY = -2;

                ResetBoard();
            }
        }
        else if (launchPressed == 1 &&
                 previousLaunchPressed == 0 &&
                 launched == 0 &&
                 gameWon == 0)
        {
            ballX = paddleX + 10;
            ballY = 105;

            velocityX = 1;
            velocityY = -2;
            launched = 1;
        }

        previousLaunchPressed = launchPressed;
        previousResetPressed = resetPressed;

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

            // Brick Collisions

            for (i = 0; i < 15; i++)
            {
                if (brickActive[i] == 0)
                {
                    continue;
                }

                if (nextBallX + 2 >= bricks[i].xMin &&
                    nextBallX - 2 <= bricks[i].xMax &&
                    nextBallY + 2 >= bricks[i].yMin &&
                    nextBallY - 2 <= bricks[i].yMax)
                {
                    // Determine Which Side the Ball Came From

                    hitHorizontal = 0;
                    hitVertical = 0;

                    if (ballX + 2 < bricks[i].xMin ||
                        ballX - 2 > bricks[i].xMax)
                    {
                        hitHorizontal = 1;
                    }

                    if (ballY + 2 < bricks[i].yMin ||
                        ballY - 2 > bricks[i].yMax)
                    {
                        hitVertical = 1;
                    }

                    // Reverse the Appropriate Direction

                    if (hitHorizontal == 1)
                    {
                        velocityX = -velocityX;
                    }

                    if (hitVertical == 1)
                    {
                        velocityY = -velocityY;
                    }

                    // Fallback for an Unexpected Overlap

                    if (hitHorizontal == 0 &&
                        hitVertical == 0)
                    {
                        velocityY = -velocityY;
                    }

                    // Keep the Ball at Its Previous Safe Position
                    // It Moves Away on the Next Update

                    nextBallX = ballX;
                    nextBallY = ballY;

                    // Remove the Brick

                    brickActive[i] = 0;
                    bricksRemaining--;

                    Graphics_setForegroundColor(
                        &g_sContext, GRAPHICS_COLOR_BLACK);

                    Graphics_fillRectangle(
                        &g_sContext, &bricks[i]);

                    // Update Only the Two Counter Digits

                    DrawBrickCount();

                    // Check for a Win

                    if (bricksRemaining == 0)
                    {
                        launched = 0;
                        gameWon = 1;

                        Graphics_setForegroundColor(
                            &g_sContext, GRAPHICS_COLOR_WHITE);

                        Graphics_drawStringCentered(
                            &g_sContext,
                            (int8_t *)"YOU WIN",
                            AUTO_STRING_LENGTH,
                            64,
                            65,
                            OPAQUE_TEXT
                        );

                        Graphics_drawStringCentered(
                            &g_sContext,
                            (int8_t *)"S2 TO RESET",
                            AUTO_STRING_LENGTH,
                            64,
                            80,
                            OPAQUE_TEXT
                        );
                    }

                    // Handle One Brick Per Update

                    break;
                }
            }

            // Paddle Collision

            if (velocityY > 0 &&
                ballY + 2 < paddle.yMin &&
                nextBallY + 2 >= paddle.yMin &&
                nextBallX + 2 >= paddle.xMin &&
                nextBallX - 2 <= paddle.xMax)
            {
                nextBallY = paddle.yMin - 3;
                velocityY = -velocityY;
            }

            // Missing the Paddle Resets the Whole Board
            // Reset Before the Ball Reaches the Counter Text

            if (nextBallY + 2 > paddle.yMax)
            {
                launched = 0;
                gameWon = 0;

                velocityX = 1;
                velocityY = -2;

                ResetBoard();
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

// Reset and Draw All Bricks

void ResetBoard(void)
{
    unsigned int row;
    unsigned int column;
    unsigned int index;

    Graphics_setBackgroundColor(
        &g_sContext, GRAPHICS_COLOR_BLACK);

    Graphics_clearDisplay(&g_sContext);

    bricksRemaining = 15;

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
            index = row * 5 + column;

            bricks[index].xMin = 5 + column * 24;
            bricks[index].xMax = bricks[index].xMin + 21;

            bricks[index].yMin = 10 + row * 12;
            bricks[index].yMax = bricks[index].yMin + 8;

            brickActive[index] = 1;

            Graphics_fillRectangle(
                &g_sContext, &bricks[index]);
        }
    }

    // Draw the Counter Label Once Per Reset

    Graphics_setForegroundColor(
        &g_sContext, GRAPHICS_COLOR_WHITE);

    Graphics_drawString(
        &g_sContext,
        (int8_t *)"Bricks:",
        AUTO_STRING_LENGTH,
        34,
        119,
        OPAQUE_TEXT
    );

    DrawBrickCount();
}

// Update Only the Two Counter Digits

void DrawBrickCount(void)
{
    char countText[3];

    countText[0] = '0' + bricksRemaining / 10;
    countText[1] = '0' + bricksRemaining % 10;
    countText[2] = '\0';

    Graphics_setForegroundColor(
        &g_sContext, GRAPHICS_COLOR_WHITE);

    Graphics_drawString(
        &g_sContext,
        (int8_t *)countText,
        AUTO_STRING_LENGTH,
        82,
        119,
        OPAQUE_TEXT
    );
}

// Timer_A0 Interrupt

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
{
    updateGame = 1;
}
