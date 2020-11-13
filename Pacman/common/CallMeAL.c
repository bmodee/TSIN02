
// Super simple OpenAL based sound player by Ingemar Ragnemalm 2014.
// Based on the "gorax" demo (link below) and on my earlier OpenAL
// player in the SAT3 library.

// Initialize with InitCallMeAl(), where you also specify how many
// channels you want. The channels are stored internally, referenced by number (0 based).
// Sounds must be WAV files compatible with the loader below. No complex multi-chunk files.
// Load sounds with LoadSound(). You retain the reference.
// Play sounds with PlaySound() to play in any free channel.
// Play with PlaySoundInChannel() to play in a specific channel.
// Stop playing sounds with StopSound() or StopAllSounds().
// Finally, clean up with HaltCallMeAL().
// On the Mac, if you use an application bundle, the default path is the "Resources" folder.

// Updated 2016, fixing a bug in PlaySoundInChannel.

#include "CallMeAL.h"

// Slightly modified by Ingemar to work on the Mac
// and to play until done, not a fixed time.
// Should be easy to get working (again) on other platforms.
// Originally found here:
// http://www.gamedev.net/community/forums/topic.asp?topic_id=505152&whichpage=1&#3296091

/*
WARNING: THIS IS ONLY A *VERY* BASIC WAVE HEADER
RIFF files (avis, waves, etc.) can have multiple headers and chunks of data,
including 'JUNK' headers which can basically be used to store messages.
Google some documentation on RIFF formats if you want to read every possible
wave file, but trust me when I say it takes a fair bit more code than this to
get something decent working.
*/

typedef struct
{
  char  riff[4];//'RIFF'
  unsigned int riffSize;
  char  wave[4];//'WAVE'
  char  fmt[4];//'fmt '
  unsigned int fmtSize;
  unsigned short format;
  unsigned short channels;
  unsigned int samplesPerSec;
  unsigned int bytesPerSec;
  unsigned short blockAlign;
  unsigned short bitsPerSample;
  char  data[4];//'data'
  unsigned int dataSize;
} BasicWAVEHeader;

typedef struct
{
  char  riff[4];//'RIFF'
  unsigned int riffSize;
  char  wave[4];//'WAVE'
  char  fmt[4];//'fmt '
  unsigned int fmtSize;
  unsigned short format;
  unsigned short channels;
  unsigned int samplesPerSec;
  unsigned int bytesPerSec;
  unsigned short blockAlign;
  unsigned short bitsPerSample;
} AltWAVEHeader;


// Slightly smarter WAV reader:
// First comes a TopHeader. Must be OK!
// Then comes TheChunkHeader. Check for "data" or "fmt ".

typedef struct
{
  char  riff[4];//'RIFF'
  unsigned int riffSize;
  char  wave[4];//'WAVE'
} TopHeader;

typedef struct
{
  unsigned short format;
  unsigned short channels;
  unsigned int samplesPerSec;
  unsigned int bytesPerSec;
  unsigned short blockAlign;
  unsigned short bitsPerSample;
} FmtHeader;

typedef struct
{
  char title[4];//'data', 'FLLR' etc
  unsigned int dataSize;
} TheChunkHeader;

#ifdef __APPLE__
// Mac specific: Make the resources folder the current directory.
static void home()
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX))
	{
		// error!
		return;
	}
	CFRelease(resourcesURL);

	chdir(path);
	printf("Current Path: %s\n", path);
}
#endif

ALCdevice* device = 0;
ALCcontext* context = 0;
int numChannels = 0;
ALuint *channel;

int InitCallMeAL(int channels)
{
#ifdef __APPLE__
  home();
#endif

  printf("Initialising Open AL...\n");
  device = alcOpenDevice(0);
  if (device)
  {
    context = alcCreateContext(device,0);
    if (context)
    {
      alcMakeContextCurrent(context);
      
      channel = malloc(sizeof(ALuint) * channels);
      alGenSources(channels,channel);
      numChannels = channels;
      
      return 1;
    }
    alcCloseDevice(device);
  }
  return 0;
}

//WARNING: This Doesn't Check To See If These Pointers Are Valid
void HaltCallMeAL()
{
  alDeleteSources(numChannels, channel);

  alcMakeContextCurrent(0);
  alcDestroyContext(context);
  alcCloseDevice(device);
}

//WARNING: This Doesn't Check To See If These Pointers Are Valid
static char* readWAVE2(char* filename, FmtHeader* header, long *dataSize)
{
	char* buffer = NULL;
	char *dummyBuffer = NULL;
	TopHeader top;
	TheChunkHeader chunk;
	char readFmt = 0;
	FILE* file = fopen(filename,"rb");
	printf("Reading File...\n");
	if (!file) return 0;

	
	if (fread(&top,sizeof(TopHeader),1,file))
	{
		if (!(//these things *must* be valid with this basic header
		memcmp("RIFF",top.riff,4) ||
		memcmp("WAVE",top.wave,4)))
		{
			for (;1;)
			{
				if (fread(&chunk,sizeof(TheChunkHeader),1,file))
				{
					printf("Chunk title = %c%c%c%c.\n", chunk.title[0], chunk.title[1], chunk.title[2], chunk.title[3]);
					if (!memcmp("fmt ",chunk.title,4))
					{
						printf("Found fmt!\n");
						printf("Data size = %d\n", chunk.dataSize);
						printf("Data size should be = %ld\n", sizeof(FmtHeader));
						if (!fread(header,chunk.dataSize,1,file)) return NULL;
						readFmt = 1;
						if (buffer)
							return buffer;
					}
					else
					if (!memcmp("data",chunk.title,4))
					{
						printf("Found data!\n");
						printf("Data size = %d\n", chunk.dataSize);
						buffer = (char*)malloc(chunk.dataSize);
						if (!fread(buffer,chunk.dataSize,1,file)) return NULL;
						printf("I hope we already have the format...\n");
						*dataSize = chunk.dataSize;
						if (readFmt)
							return buffer;
					}
					else
					{
						printf("Found something else! %c%c%c%c Skipping %d bytes.\n", chunk.title[0], chunk.title[1], chunk.title[2], chunk.title[3], chunk.dataSize);
						dummyBuffer = (char*)malloc(chunk.dataSize);
						if (!fread(dummyBuffer,chunk.dataSize,1,file)) return NULL;
						free(dummyBuffer);
						dummyBuffer = NULL;
					}

				}
				else
					return NULL;
			}
		}
		else
		{
			printf("Format not supported for %s. Sorry!\n", filename);
			printf("One of these mismatch:\n");
			printf("RIFF = %c%c%c%c\n", top.riff[0], top.riff[1], top.riff[2], top.riff[3]);
			printf("WAVE = %c%c%c%c\n", top.wave[0], top.wave[1], top.wave[2], top.wave[3]);
		}
	}
	else
		printf("Failed to read file %s\n", filename);
	fclose(file);
	return 0;
}

static ALuint createBufferFromWave2(char* data, FmtHeader header, long dataSize)
{
  ALuint buffer = 0;
  ALuint format = 0;
  printf("Creating Buffer...\n");
  switch (header.bitsPerSample)
  {
    case 8:
      format = (header.channels == 1) ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
      break;
    case 16:
      format = (header.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
      break;
    default:
      //last time I checked, Open AL didn't support anything else
      printf("Sorry, only mono8, mono16, stereo8 and stereo16 are supported!\n");
      return 0;
  }
  
  alGenBuffers(1,&buffer);
  alBufferData(buffer, format, data, dataSize, header.samplesPerSec);
  return buffer;
}

ALuint LoadSound(char* filename)
{
//  BasicWAVEHeader
  FmtHeader header;
  long dataSize;
  char* data = readWAVE2(filename, &header, &dataSize);
  
  if (data)
  {
    //Now We've Got A Wave In Memory, Time To Turn It Into A Usable Buffer
    ALuint buffer = createBufferFromWave2(data, header, dataSize);
    free(data);
    return buffer;
  }
  else
  {
    printf("FAILED TO LOAD %s\n", filename);
    return -1;
  }
}

void PlaySound(ALuint buffer)
{
	int i;
	ALint status;
	
	// Find a free channel
	for (i = 0; i < numChannels; i++)
	{
		alGetSourcei (channel[i], AL_SOURCE_STATE, &status);
		if (status != AL_PLAYING)
		{
			alSourceStop(channel[i]);
			alSourcei(channel[i], AL_BUFFER, buffer);
			alSourcePlay(channel[i]);
			return;
		}
	}
}

void PlaySoundInChannel(ALuint buffer, int ch)
{
	if (ch < 0) return;
	if (ch >= numChannels) return;
	
	alSourceStop(channel[ch]);
	alSourcei(channel[ch], AL_BUFFER, buffer);
	alSourcePlay(channel[ch]);
}

ALint GetChannelStatus(int ch)
{
	ALint status;
	if (ch < 0) return 0;
	if (ch >= numChannels) return 0;
	alGetSourcei (channel[ch], AL_SOURCE_STATE, &status);
	return status;
}

char ChannelIsPlaying(int ch)
{
	ALint status;
	if (ch < 0) return 0;
	if (ch >= numChannels) return 0;
	alGetSourcei (channel[ch], AL_SOURCE_STATE, &status);
	return (status == AL_PLAYING);
}

void StopSound(int ch)
{
	if (ch < 0) return;
	if (ch >= numChannels) return;
	alSourceStop(channel[ch]);
}

void StopAllSounds()
{
	int i;
	for (i = 0; i < numChannels; i++)
	{
		alSourceStop(channel[i]);
	}
}
