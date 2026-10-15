#include "graphics.h"

void Initialize_Graphics(Graphics_Context *context)
{
    // Set screen orientation
    Crystalfontz128x128_SetOrientation(0);

    // Connect the graphics context to the LCD
    Graphics_initContext(context, &g_sCrystalfontz128x128);

    // Set text and background colors
    Graphics_setBackgroundColor(context, GRAPHICS_COLOR_BLACK);
    Graphics_setForegroundColor(context, GRAPHICS_COLOR_WHITE);

    // Select the font
    GrContextFontSet(context, &g_sFontFixed6x8);

    // Clear the screen
    Graphics_clearDisplay(context);

    // Turn on the backlight
    P3DIR |= BIT6;
    P3OUT |= BIT6;
}
