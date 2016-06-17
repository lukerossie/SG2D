#include "../include/util.hpp"
#include "../include/sound.hpp"
#include "../include/graphics.hpp"
#include "../include/net.hpp"

#include <cstdio>

#ifdef SG2D_TEST
void util_test();
void graphics_test();
void net_test();
void input_test();
void sound_test();
void util_test();
#endif

void onQuit()
{
    //on linux:
    //if i dont do this i get a memory corruption error on exit
    //if i DO, i get a segfault in my graphics driver, UNLESS Im running as superuser
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDLNet_Quit();
    IMG_Quit();
    Mix_Quit();
    TTF_Quit();
    SDL_Quit();
}
//flags are SDL_CreateWindow flags NOT SDL_Init
Vec2 init( char const *title, int width, int height, int flags )
{
#ifdef SG2D_TEST
	util_test();
	graphics_test();
	net_test();
	input_test();
	sound_test();
#endif

	Vec2 wh=initGraphics( title, width, height, flags );
	initNet();
	initSound();

    //on linux:
    //if i dont do this i get a memory corruption error on exit
    //if i DO, i get a segfault in my graphics driver, UNLESS Im running as superuser
    #ifdef SG2D_DOCLEANUP
    atexit(onQuit);
    #endif
	return wh;
}
