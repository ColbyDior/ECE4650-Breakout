/*
 * sound.c
 *
 *  Created on: Oct 15, 2026
 *      Author: Colby
 */

#include <msp430fr5994.h>
#include <stdint.h>
#include "sound.h"

// Simplified Ride of the Valkyries Motif
// Frequencies in Hz; durations in approximately 10 ms units
// A frequency of zero represents a rest.

static const unsigned int victoryNotes[] =
{
    370, 494, 587, 740, 587, 740, 0,
    587, 740, 880, 740, 587, 494, 0,
    440, 587, 740, 880, 740, 880, 0,
    740, 880, 988, 880, 740, 587, 0,
    370, 494, 587, 740, 587, 740, 0,
    587, 494, 370, 494
};

static const unsigned char victoryDurations[] =
{
    12, 24, 12, 36, 12, 48, 12,
    12, 24, 12, 24, 12, 48, 18,
    12, 24, 12, 36, 12, 48, 12,
    12, 24, 12, 24, 12, 48, 18,
    12, 24, 12, 36, 12, 48, 12,
    24, 24, 24, 72
};

#define VICTORY_NOTE_COUNT (sizeof(victoryNotes) / sizeof(victoryNotes[0]))

// Sound Modes
// 0 = Silent, 1 = Brick Beep, 2 = Victory Song

static volatile unsigned char soundReady = 0;
static volatile unsigned char soundMode = 0;
static volatile unsigned char noteIndex = 0;
static volatile unsigned char noteGap = 0;
static volatile unsigned int ticksRemaining = 0;

// Private Function Prototypes

static void SetTone(unsigned int frequency);
static void SilenceBuzzer(void);

// Initialize the Buzzer and Sound Timers

unsigned char SoundInit(void)
{
    unsigned int attempts;

    soundReady = 0;
    soundMode = 0;

    // Start With the Buzzer Pin Low

    P3SEL0 &= ~BIT7;
    P3SEL1 &= ~BIT7;
    P3OUT &= ~BIT7;
    P3DIR |= BIT7;

    TB0CTL = MC__STOP | TBCLR;
    TB0EX0 = 0;
    TB0CCTL6 = 0;

    // Configure the LaunchPad's Low-Frequency Crystal
    // Preserve the Existing MCLK and SMCLK Settings

    PJSEL0 |= BIT4 | BIT5;
    PJSEL1 &= ~(BIT4 | BIT5);

    CSCTL0 = CSKEY;
    CSCTL2 = (CSCTL2 & ~SELA_7) | SELA__LFXTCLK;
    CSCTL3 = (CSCTL3 & ~0x0700u) | DIVA__1;
    CSCTL4 |= LFXTDRIVE_3;
    CSCTL4 &= ~LFXTOFF;

    // Bounded Startup Wait So a Crystal Fault Cannot Hang the Game

    for (attempts = 0; attempts < 50000; attempts++)
    {
        CSCTL5 &= ~LFXTOFFG;
        SFRIFG1 &= ~OFIFG;

        __delay_cycles(100);

        if (!(CSCTL5 & LFXTOFFG))
        {
            soundReady = 1;
            break;
        }
    }

    CSCTL0_H = 0;

    if (soundReady == 0)
    {
        return 0;
    }

    // Timer_A1 Handles Note Durations
    // 328 ACLK Counts Is Approximately 10 ms

    TA1CTL = MC__STOP | TACLR;
    TA1EX0 = 0;
    TA1CCR0 = 327;
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL__ACLK | MC__UP | TACLR;

    return 1;
}

// Generate a Tone With Hardware PWM

static void SetTone(unsigned int frequency)
{
    unsigned int period;

    SilenceBuzzer();

    if (frequency == 0)
    {
        return;
    }

    period = (32768UL + frequency / 2) / frequency;

    TB0CCR0 = period - 1;
    TB0CCR6 = period / 2;
    TB0CCTL6 = OUTMOD_7;

    P3SEL1 &= ~BIT7;
    P3SEL0 |= BIT7;

    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;
}

// Stop PWM and Hold the Buzzer Pin Low

static void SilenceBuzzer(void)
{
    TB0CTL = MC__STOP | TBCLR;
    TB0CCTL6 = 0;

    P3SEL0 &= ~BIT7;
    P3SEL1 &= ~BIT7;
    P3OUT &= ~BIT7;
}

// Play a Short Brick-Hit Beep

void PlayBrickSound(void)
{
    unsigned int interruptState;

    interruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    if (soundReady == 1 && soundMode != 2)
    {
        soundMode = 1;
        ticksRemaining = 6;
        noteGap = 0;

        SetTone(1500);
    }

    if (interruptState != 0)
    {
        __enable_interrupt();
    }
}

// Start the Victory Melody

void PlayVictorySong(void)
{
    unsigned int interruptState;

    interruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    if (soundReady == 1)
    {
        soundMode = 2;
        noteIndex = 0;
        noteGap = 0;
        ticksRemaining = victoryDurations[0];

        SetTone(victoryNotes[0]);
    }

    if (interruptState != 0)
    {
        __enable_interrupt();
    }
}

// Cancel Any Sound

void StopSound(void)
{
    unsigned int interruptState;

    interruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    soundMode = 0;
    ticksRemaining = 0;
    noteGap = 0;

    SilenceBuzzer();

    if (interruptState != 0)
    {
        __enable_interrupt();
    }
}

// Advance Sound Without Delays in the Game Loop

#pragma vector = TIMER1_A0_VECTOR
__interrupt void SoundTimer_ISR(void)
{
    if (soundMode == 0)
    {
        return;
    }

    if (ticksRemaining > 0)
    {
        ticksRemaining--;
    }

    if (ticksRemaining != 0)
    {
        return;
    }

    // Finish the Brick Beep

    if (soundMode == 1)
    {
        SilenceBuzzer();
        soundMode = 0;
        return;
    }

    // Insert a Short Gap Between Melody Notes

    if (noteGap == 0)
    {
        SilenceBuzzer();
        noteGap = 1;
        ticksRemaining = 2;
        return;
    }

    // Start the Next Note

    noteIndex++;

    if (noteIndex >= VICTORY_NOTE_COUNT)
    {
        SilenceBuzzer();
        soundMode = 0;
        return;
    }

    noteGap = 0;
    ticksRemaining = victoryDurations[noteIndex];

    SetTone(victoryNotes[noteIndex]);
}


