/*
 * levels.h
 *
 *  Created on: Oct 18, 2026
 *      Author: Colby
 */

#ifndef LEVELS_H_
#define LEVELS_H_

#define NUM_LEVELS 4
#define LEVEL_ROWS 5
#define LEVEL_COLUMNS 5
#define BRICK_COUNT (LEVEL_ROWS * LEVEL_COLUMNS)

// Each level uses a five-column grid; 1 places a brick and 0 leaves a gap
static const unsigned char levelLayouts[NUM_LEVELS][LEVEL_ROWS][LEVEL_COLUMNS] =
{
    // Level 1: three solid rows
    {
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0}
    },

    // Level 2: alternating full and spaced rows
    {
        {1, 1, 1, 1, 1},
        {1, 0, 1, 0, 1},
        {1, 1, 1, 1, 1},
        {0, 1, 0, 1, 0},
        {1, 0, 1, 0, 1}
    },

    // Level 3: a diamond
    {
        {0, 0, 1, 0, 0},
        {0, 1, 1, 1, 0},
        {1, 1, 1, 1, 1},
        {0, 1, 1, 1, 0},
        {0, 0, 1, 0, 0}
    },

    // Level 4: five solid rows
    {
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1}
    }
};

#endif
