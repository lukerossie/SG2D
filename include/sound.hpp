#pragma once

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2_mixer/SDL_mixer.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#endif

void initSound();


class Sound{
	public:
	bool loop;
	Mix_Chunk *m;
	int channel;
	
	Sound( Mix_Chunk *m_p, bool loop_p=false );
	
	void play();
	void pause();
	void resume();
};