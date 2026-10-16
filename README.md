# MSP430 Breakout

**Authors:** Colby Ferrell and Hayden Murray  
**Course:** ECE 4650

Breakout game developed for the MSP430FR5994 LaunchPad and Educational BoosterPack MKII.

The player uses the joystick to move a paddle and bounce a ball into colored bricks. Clearing all 15 bricks wins the game. The player starts with three lives and loses one whenever the ball passes the paddle.

## Hardware and Software

- MSP-EXP430FR5994 LaunchPad
- BOOSTXL-EDUMKII Educational BoosterPack
- USB connection to a computer
- Code Composer Studio 12
- GitHub Desktop for version control

## Controls

| Control | Action |
| --- | --- |
| Joystick horizontal movement | Move the paddle |
| BoosterPack S1 | Launch the ball |
| BoosterPack S2 | Reset the game |

After a missed ball, press S1 to launch again. After winning or losing all lives, press S2 to reset.

## Features

- Joystick-controlled paddle with calibrated movement limits
- Timer-driven game updates
- Ball collisions with walls, paddle, and bricks
- Remaining brick counter above the play area
- Three lives represented by dots below the play area
- Short buzzer sound when a brick is hit
- Simplified Ride of the Valkyries melody after a win

## Project Files

The CCS project is inside the ECE4650_Breakout folder.

| File or Folder | Purpose |
| --- | --- |
| main.c | Hardware initialization, controls, game logic, and display updates |
| graphics.c and graphics.h | Graphics setup and drawing functions |
| sound.c and sound.h | Buzzer control and music playback |
| GrLib | Graphics library |
| LcdDriver | LCD driver |
| targetConfigs | Target configuration for debugging |
| lnk_msp430fr5994.cmd | Linker memory configuration |

## Import and Run

1. Clone this repository to your computer using GitHub Desktop.
2. Open Code Composer Studio 12.
3. Select Project > Import CCS Projects.
4. Browse to the ECE4650_Breakout folder inside the cloned repository.
5. Select the discovered project.
6. Leave Copy projects into workspace unchecked to work directly in the repository.
7. Finish the import.
8. Connect the LaunchPad with the BoosterPack attached.
9. Build the project.
10. Start a debug session to load the program, then click Resume.
11. Use the joystick to move the paddle and press BoosterPack S1 to launch.

## Hardware Check

- The paddle reaches both screen edges.
- S1 launches the ball.
- Brick collisions remove bricks and update the counter.
- Brick hits produce a short sound.
- Missing the ball removes one life dot.
- Losing all lives displays GAME OVER.
- Clearing all bricks displays YOU WIN and plays the victory melody.
- S2 resets the board and stops sound playback.

## References and Credits

This project began with the LCD setup from our stopwatch lab and used Thomas Stoeckert's MSPBreakout project as an instructor-provided reference for the game structure and graphics functions.

- Original project: [MSPBreakout by Thomas Stoeckert](https://github.com/thomasstoeckert/MSPBreakout)
- Project explanation: [Breakout for the MSP430](https://thomasstoeckert.us/project/msp430-breakout)

The reference project targets the MSP430FR6989. Our version targets the MSP430FR5994 and includes board-specific pin configuration, calibrated joystick input, limited paddle movement per update, a revised display layout, brick-hit sounds, and victory music.

The graphics library and LCD driver files include Texas Instruments software. Original copyright and license notices should remain with any reused files.

The victory melody is a simplified adaptation of Richard Wagner's Ride of the Valkyries.