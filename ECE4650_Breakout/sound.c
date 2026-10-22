#include <msp430fr5994.h>
#include <stdint.h>
#include "sound.h"

// Ride of the Valkyries: final victory only.
static const unsigned int victoryNotes[] =
{
    370, 494, 587, 740, 587, 740, 0,
    587, 740, 880, 740, 587, 494, 0,
    440, 587, 740, 880, 740, 880, 0,
    740, 880, 988, 880, 740, 587, 0,
    370, 494, 587, 740, 587, 740, 0,
    587, 494, 370, 494
};

// Durations are approximately 10 ms per tick.
static const unsigned char victoryDurations[] =
{
    12, 24, 12, 36, 12, 48, 12,
    12, 24, 12, 24, 12, 48, 18,
    12, 24, 12, 36, 12, 48, 12,
    12, 24, 12, 24, 12, 48, 18,
    12, 24, 12, 36, 12, 48, 12,
    24, 24, 24, 72
};

// Repeating gameplay tune.
static const unsigned int levelNotes[] =
{
    262, 330, 392, 330, 294, 349, 440, 349,
    330, 392, 494, 392, 349, 440, 523, 392
};

static const unsigned char levelDurations[] =
{
    14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 28, 14
};

// Short victory jingle for regular levels.
static const unsigned int clearNotes[] =
{
    523, 659, 784, 1047
};

static const unsigned char clearDurations[] =
{
    12, 12, 12, 72
};

#define VICTORY_NOTE_COUNT \
    (sizeof(victoryNotes) / sizeof(victoryNotes[0]))

#define LEVEL_NOTE_COUNT \
    (sizeof(levelNotes) / sizeof(levelNotes[0]))

#define CLEAR_NOTE_COUNT \
    (sizeof(clearNotes) / sizeof(clearNotes[0]))

#define SILENT         0
#define LEVEL_MUSIC    1
#define LEVEL_VICTORY  2
#define FINAL_VICTORY  3

static volatile unsigned char soundReady = 0;
static volatile unsigned char soundMode = SILENT;
static volatile unsigned char noteIndex = 0;
static volatile unsigned char noteGap = 0;
static volatile unsigned char brickTicks = 0;
static volatile unsigned int ticksRemaining = 0;

static const unsigned int * volatile activeNotes = 0;
static const unsigned char * volatile activeDurations = 0;
static volatile unsigned int activeCount = 0;

static void SetTone(unsigned int frequency);
static void SilenceBuzzer(void);

static void StartSong(
    unsigned char mode,
    const unsigned int *notes,
    const unsigned char *durations,
    unsigned int count
);

unsigned char SoundInit(void)
{
    unsigned int attempts;

    soundReady = 0;
    soundMode = SILENT;

    // Start with the buzzer pin low.
    P3SEL0 &= ~BIT7;
    P3SEL1 &= ~BIT7;
    P3OUT &= ~BIT7;
    P3DIR |= BIT7;

    TB0CTL = MC__STOP | TBCLR;
    TB0EX0 = 0;
    TB0CCTL6 = 0;

    // Configure the low-frequency crystal.
    // Preserve the existing MCLK and SMCLK settings.
    PJSEL0 |= BIT4 | BIT5;
    PJSEL1 &= ~(BIT4 | BIT5);

    CSCTL0 = CSKEY;
    CSCTL2 = (CSCTL2 & ~SELA_7) | SELA__LFXTCLK;
    CSCTL3 = (CSCTL3 & ~0x0700u) | DIVA__1;
    CSCTL4 |= LFXTDRIVE_3;
    CSCTL4 &= ~LFXTOFF;

    // Limit startup time so a crystal fault cannot hang the game.
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

    if (!soundReady)
    {
        return 0;
    }

    // Timer_A1 advances sound approximately every 10 ms.
    TA1CTL = MC__STOP | TACLR;
    TA1EX0 = 0;
    TA1CCR0 = 327;
    TA1CCTL0 = CCIE;
    TA1CTL = TASSEL__ACLK | MC__UP | TACLR;

    return 1;
}

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
    TB0CCR6 = period / 30;
    TB0CCTL6 = OUTMOD_7;

    P3SEL1 &= ~BIT7;
    P3SEL0 |= BIT7;

    TB0CTL = TBSSEL__ACLK | MC__UP | TBCLR;
}

static void SilenceBuzzer(void)
{
    TB0CTL = MC__STOP | TBCLR;
    TB0CCTL6 = 0;

    P3SEL0 &= ~BIT7;
    P3SEL1 &= ~BIT7;
    P3OUT &= ~BIT7;
}

static void StartSong(
    unsigned char mode,
    const unsigned int *notes,
    const unsigned char *durations,
    unsigned int count
)
{
    unsigned int interruptState;

    interruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    if (soundReady)
    {
        activeNotes = notes;
        activeDurations = durations;
        activeCount = count;

        noteIndex = 0;
        noteGap = 0;
        brickTicks = 0;
        ticksRemaining = durations[0];
        soundMode = mode;

        SetTone(notes[0]);
    }

    if (interruptState)
    {
        __enable_interrupt();
    }
}

void PlayLevelMusic(void)
{
    StartSong(
        LEVEL_MUSIC,
        levelNotes,
        levelDurations,
        LEVEL_NOTE_COUNT
    );
}

void PlayLevelVictorySong(void)
{
    StartSong(
        LEVEL_VICTORY,
        clearNotes,
        clearDurations,
        CLEAR_NOTE_COUNT
    );
}

void PlayVictorySong(void)
{
    StartSong(
        FINAL_VICTORY,
        victoryNotes,
        victoryDurations,
        VICTORY_NOTE_COUNT
    );
}

unsigned char SoundIsPlaying(void)
{
    return soundMode != SILENT || brickTicks != 0;
}

void PlayBrickSound(void)
{
    unsigned int interruptState;

    interruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    if (soundReady &&
        (soundMode == SILENT || soundMode == LEVEL_MUSIC))
    {
        brickTicks = 6;
        SetTone(1500);
    }

    if (interruptState)
    {
        __enable_interrupt();
    }
}

void StopSound(void)
{
    unsigned int interruptState;

    interruptState = __get_SR_register() & GIE;
    __disable_interrupt();

    soundMode = SILENT;
    brickTicks = 0;
    ticksRemaining = 0;
    noteGap = 0;

    SilenceBuzzer();

    if (interruptState)
    {
        __enable_interrupt();
    }
}

#pragma vector = TIMER1_A0_VECTOR
__interrupt void SoundTimer_ISR(void)
{
    // Briefly interrupt music for brick hits.
    if (brickTicks)
    {
        brickTicks--;

        if (!brickTicks)
        {
            if (soundMode != SILENT && !noteGap)
            {
                SetTone(activeNotes[noteIndex]);
            }
            else
            {
                SilenceBuzzer();
            }
        }

        return;
    }

    if (soundMode == SILENT)
    {
        return;
    }

    if (ticksRemaining)
    {
        ticksRemaining--;
    }

    if (ticksRemaining)
    {
        return;
    }

    // Small gap between notes.
    if (!noteGap)
    {
        SilenceBuzzer();
        noteGap = 1;
        ticksRemaining = 2;
        return;
    }

    noteIndex++;

    if (noteIndex >= activeCount)
    {
        if (soundMode == LEVEL_MUSIC)
        {
            noteIndex = 0;
        }
        else
        {
            SilenceBuzzer();
            soundMode = SILENT;
            return;
        }
    }

    noteGap = 0;
    ticksRemaining = activeDurations[noteIndex];
    SetTone(activeNotes[noteIndex]);
}
