#include "../include/sound.hpp"
#include <cstdlib>
#include <iostream>
#include <cstdio>

namespace
{
	int soundCounter=0;	
}

void initSound()
{
	int audioRate = 22050;
	Uint16 audioFormat = AUDIO_S16SYS;
	int audioChannels = 2;
	int audioBuffers = 4096;
	
	if( Mix_OpenAudio(audioRate, audioFormat, audioChannels, audioBuffers) != 0 ) 
	{
		std::cout<<"Unable to initialize audio:\n"<<Mix_GetError();
		exit(1);
	}
}

Sound::Sound( Mix_Chunk *m_p, bool loop_p )
{	
	m=m_p;
	loop=loop_p;
	channel=soundCounter++;
}

void Sound::play()
{
	Mix_PlayChannel( channel, m, 0 );
}
void Sound::pause()
{
	Mix_Pause( channel );
}
void Sound::resume()
{
	Mix_Resume( channel );
}

#ifdef SG2D_TEST
void sound_test()
{
}
#endif