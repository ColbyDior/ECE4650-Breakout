#include <msp430fr5994.h>
#include <stdint.h>
#include "graphics.h"
#include "sound.h"

// Game settings: number of bricks, starting lives, and maximum paddle movement per update
#define BRICK_COUNT 15
#define STARTING_LIVES 3
#define PADDLE_STEP 6

// Store the graphics context, brick positions, and active brick states
Graphics_Context g_sContext;
Graphics_Rectangle bricks[BRICK_COUNT];
unsigned char brickActive[BRICK_COUNT];

// The timer requests game updates; the counters track remaining bricks and lives
volatile unsigned char updateGame = 0;
unsigned int bricksRemaining = BRICK_COUNT;
unsigned int livesRemaining = STARTING_LIVES;

// Functions for resetting the board and updating the display
void ResetBoard(void);
void DrawBrickCount(void);
void DrawLives(void);
void DrawEndMessage(unsigned char won);

int main(void)
{
    // Joystick input and brick loop index
    unsigned int joystickValue;
    unsigned int i;

    // Paddle position, ball position, and ball movement per update
    int paddleX = 54;
    int targetPaddleX = 54;
    int ballX = 64;
    int ballY = 105;
    int nextBallX;
    int nextBallY;
    int velocityX = 1;
    int velocityY = -2;

    // Track game state, button presses, and collision direction
    unsigned char launched = 0;
    unsigned char gameWon = 0;
    unsigned char gameOver = 0;
    unsigned char launchPressed;
    unsigned char resetPressed;
    unsigned char previousLaunchPressed = 0;
    unsigned char previousResetPressed = 0;
    unsigned char hitHorizontal;
    unsigned char hitVertical;

    Graphics_Rectangle paddle = {54, 110, 73, 114};

    // Stop the watchdog and unlock the GPIO pins
    WDTCTL = WDTPW | WDTHOLD;
    PM5CTL0 &= ~LOCKLPM5;

    // Set the SMCLK divider to /1, then lock the clock registers
    CSCTL0 = CSKEY;
    CSCTL3 &= ~(BIT4 | BIT5 | BIT6);
    CSCTL0_H = 0;

    // Initialize the LCD, graphics settings, and sound hardware
    Crystalfontz128x128_Init();
    Initialize_Graphics(&g_sContext);
    SoundInit();

    // Configure P1.2 as analog input A2 for horizontal joystick movement
    P1SEL0 |= BIT2;
    P1SEL1 |= BIT2;

    // Configure ADC12 for individual 12-bit conversions of the joystick input
    ADC12CTL0 &= ~ADC12ENC;
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;
    ADC12CTL2 = ADC12RES_2;
    ADC12CTL3 = 0;
    ADC12MCTL0 = ADC12INCH_2;
    ADC12CTL0 |= ADC12ENC;

    // Configure BoosterPack S1 and S2 as active-low inputs with pull-up resistors
    P4SEL0 &= ~(BIT3 | BIT2);
    P4SEL1 &= ~(BIT3 | BIT2);
    P4DIR &= ~(BIT3 | BIT2);
    P4OUT |= BIT3 | BIT2;
    P4REN |= BIT3 | BIT2;

    // Draw the starting board with the ball resting above the paddle
    ResetBoard();
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_fillRectangle(&g_sContext, &paddle);
    Graphics_fillCircle(&g_sContext, ballX, ballY, 2);

    // Timer_A0 uses SMCLK /8 and requests an update whenever it reaches CCR0
    TA0CTL = MC__STOP | TACLR;
    TA0EX0 = 0;
    TA0CCR0 = 33332;
    TA0CCTL0 = CCIE;
    TA0CTL = TASSEL__SMCLK | ID__8 | MC__UP | TACLR;

    __enable_interrupt();

    while (1)
    {
        // Run one game update when the timer sets the update flag
        if (!updateGame) continue;
        updateGame = 0;

        // Start an ADC conversion and read the joystick after it finishes
        ADC12CTL0 |= ADC12SC;
        while (ADC12CTL1 & ADC12BUSY);
        joystickValue = ADC12MEM0;

        // Erase the previous paddle and ball without clearing the entire screen
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
        Graphics_fillRectangle(&g_sContext, &paddle);
        Graphics_fillCircle(&g_sContext, ballX, ballY, 2);

        // Map the calibrated joystick range to paddle positions from 0 to 108
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
            targetPaddleX = (int)(((uint32_t)(joystickValue - 700) * 108) / 2430);
        }

        // Move toward the target by up to PADDLE_STEP pixels without overshooting
        if (paddleX < targetPaddleX)
        {
            paddleX += PADDLE_STEP;
            if (paddleX > targetPaddleX) paddleX = targetPaddleX;
        }
        else if (paddleX > targetPaddleX)
        {
            paddleX -= PADDLE_STEP;
            if (paddleX < targetPaddleX) paddleX = targetPaddleX;
        }

        paddle.xMin = paddleX;
        paddle.xMax = paddleX + 19;

        // Read S1 for launch and S2 for reset; pressed buttons read as zero
        launchPressed = !(P4IN & BIT3);
        resetPressed = !(P4IN & BIT2);

        // Reset on a new S2 press, restoring bricks, lives, and the waiting ball
        if (resetPressed && !previousResetPressed)
        {
            launched = 0;
            gameWon = 0;
            gameOver = 0;
            velocityX = 1;
            velocityY = -2;
            ResetBoard();
        }

        // Launch on a new S1 press if the ball is waiting and the game is active
        else if (launchPressed && !previousLaunchPressed && !launched && !gameWon && !gameOver)
        {
            ballX = paddleX + 10;
            ballY = 105;
            velocityX = 1;
            velocityY = -2;
            launched = 1;
        }

        // Remember button states so holding a button does not repeat its action
        previousLaunchPressed = launchPressed;
        previousResetPressed = resetPressed;

        if (launched)
        {
            // Calculate the ball's proposed position before checking collisions
            nextBallX = ballX + velocityX;
            nextBallY = ballY + velocityY;

            // Reflect the ball at the left and right edges, accounting for its radius
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

            // Reflect the ball below the brick counter to protect the top text
            if (nextBallY < 10)
            {
                nextBallY = 10;
                velocityY = -velocityY;
            }

            // Check whether the ball's proposed bounds overlap an active brick
            for (i = 0; i < BRICK_COUNT; i++)
            {
                if (!brickActive[i]) continue;

                if (nextBallX + 2 >= bricks[i].xMin && nextBallX - 2 <= bricks[i].xMax && nextBallY + 2 >= bricks[i].yMin && nextBallY - 2 <= bricks[i].yMax)
                {
                    // Use the previous position to determine which direction to reflect
                    hitHorizontal = (ballX + 2 < bricks[i].xMin || ballX - 2 > bricks[i].xMax);
                    hitVertical = (ballY + 2 < bricks[i].yMin || ballY - 2 > bricks[i].yMax);

                    if (hitHorizontal) velocityX = -velocityX;
                    if (hitVertical || !hitHorizontal) velocityY = -velocityY;

                    // Keep the ball at its previous position for this collision update
                    nextBallX = ballX;
                    nextBallY = ballY;

                    // Remove the hit brick, update the counter, and play a short tone
                    brickActive[i] = 0;
                    bricksRemaining--;

                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_fillRectangle(&g_sContext, &bricks[i]);

                    DrawBrickCount();
                    PlayBrickSound();

                    // End the round and play the victory song when all bricks are gone
                    if (bricksRemaining == 0)
                    {
                        launched = 0;
                        gameWon = 1;
                        DrawEndMessage(1);
                        PlayVictorySong();
                    }

                    // Handle only one brick collision per update
                    break;
                }
            }

            // Bounce upward when the falling ball crosses the paddle's top surface
            if (velocityY > 0 && ballY + 2 < 110 && nextBallY + 2 >= 110 && nextBallX + 2 >= paddle.xMin && nextBallX - 2 <= paddle.xMax)
            {
                nextBallY = 107;
                velocityY = -velocityY;
            }

            // Deduct a life when the ball passes below the paddle
            if (nextBallY + 2 > 114)
            {
                launched = 0;
                velocityX = 1;
                velocityY = -2;

                if (livesRemaining > 0) livesRemaining--;
                DrawLives();

                // Require a reset once all lives have been used
                if (livesRemaining == 0)
                {
                    gameOver = 1;
                    DrawEndMessage(0);
                }
            }
            else
            {
                // Accept the position after resolving collisions
                ballX = nextBallX;
                ballY = nextBallY;
            }
        }

        // Keep the waiting ball attached to the paddle until launch
        if (!launched)
        {
            ballX = paddleX + 10;
            ballY = 105;
        }

        // Draw the updated paddle and show the ball while the round is unfinished
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_fillRectangle(&g_sContext, &paddle);

        if (!gameWon && !gameOver) Graphics_fillCircle(&g_sContext, ballX, ballY, 2);
    }
}

// Stop playback, restore the counters, and redraw the complete starting board
void ResetBoard(void)
{
    unsigned int row;
    unsigned int column;
    unsigned int index;

    StopSound();

    bricksRemaining = BRICK_COUNT;
    livesRemaining = STARTING_LIVES;

    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&g_sContext);

    // Create three rows of five bricks, with a different color for each row
    for (row = 0; row < 3; row++)
    {
        if (row == 0) Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
        else if (row == 1) Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_GREEN);
        else Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLUE);

        for (column = 0; column < 5; column++)
        {
            index = row * 5 + column;

            bricks[index].xMin = 5 + column * 24;
            bricks[index].xMax = bricks[index].xMin + 21;
            bricks[index].yMin = 10 + row * 12;
            bricks[index].yMax = bricks[index].yMin + 8;
            brickActive[index] = 1;

            Graphics_fillRectangle(&g_sContext, &bricks[index]);
        }
    }

    // Place the brick counter above the board and the life indicators below it
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawString(&g_sContext, (int8_t *)"Bricks:", AUTO_STRING_LENGTH, 34, 0, OPAQUE_TEXT);
    Graphics_drawString(&g_sContext, (int8_t *)"Lives:", AUTO_STRING_LENGTH, 34, 119, OPAQUE_TEXT);

    DrawBrickCount();
    DrawLives();
}

// Convert the brick count to two digits and redraw only the changing number
void DrawBrickCount(void)
{
    char countText[3];

    countText[0] = '0' + bricksRemaining / 10;
    countText[1] = '0' + bricksRemaining % 10;
    countText[2] = '\0';

    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawString(&g_sContext, (int8_t *)countText, AUTO_STRING_LENGTH, 82, 0, OPAQUE_TEXT);
}

// Draw one white dot per remaining life and erase dots for lives already lost
void DrawLives(void)
{
    unsigned int i;

    for (i = 0; i < STARTING_LIVES; i++)
    {
        Graphics_setForegroundColor(&g_sContext, i < livesRemaining ? GRAPHICS_COLOR_WHITE : GRAPHICS_COLOR_BLACK);
        Graphics_fillCircle(&g_sContext, 77 + i * 8, 123, 2);
    }
}

// Display the round's result and the button used to start a new game
void DrawEndMessage(unsigned char won)
{
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawStringCentered(&g_sContext, (int8_t *)(won ? "YOU WIN" : "GAME OVER"), AUTO_STRING_LENGTH, 64, 65, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, (int8_t *)"S2 TO RESET", AUTO_STRING_LENGTH, 64, 80, OPAQUE_TEXT);
}

// Request an update from the main loop; drawing stays outside the interrupt
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
{
    updateGame = 1;
}
