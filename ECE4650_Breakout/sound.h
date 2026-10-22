/*
 * sound.h
 *
 *  Created on: Oct 15, 2026
 *      Author: Colby
 */
#ifndef SOUND_H_
#define SOUND_H_

unsigned char SoundInit(void);
void PlayLevelMusic(void);
void PlayBrickSound(void);
void PlayLevelVictorySong(void);
void PlayVictorySong(void);
unsigned char SoundIsPlaying(void);
void StopSound(void);

#endif
