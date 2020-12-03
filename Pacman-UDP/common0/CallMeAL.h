#ifndef YOUCANCALLMEAL
#define YOUCANCALLMEAL

#ifdef __APPLE__
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
//	#include <Carbon/Carbon.h>
//	#include <Cocoa/Cocoa.h>
	#include <CoreFoundation/CoreFoundation.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int InitCallMeAL(int channels);
void HaltCallMeAL();
ALuint LoadSound(char* filename);
ALint GetChannelStatus(int ch);
char ChannelIsPlaying(int ch);
void PlaySoundInChannel(ALuint buffer, int ch);
void PlaySound(ALuint buffer);
void StopSound(int ch);
void StopAllSounds();

#endif

