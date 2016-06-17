#pragma once

#ifdef __APPLE__
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mutex.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mutex.h>
#endif
#include <functional>
#include "util.hpp"

extern Vec2 mouseScroll;
/*
returns the state of the key (true for pressed false for released).
SDL_Keycode is a direct map to the char most of the time.
For example you can pass 'c' to see if SDLK_C is being pressed.
*/
extern std::function<void(int)> keyDownEvent;
extern std::function<void(int)> keyUpEvent;
bool key( SDL_Keycode );
extern void *onWindowEvent;
/*
Don't call this unless you're on windows.
On windows you must call this every frame.
*/
void poll();

enum class MB
{
	LEFT,
	RIGHT,
	MIDDLE,
	NONE
};
/*
If you specify a button and it isn't pressed, returns null.
If it is pressed, returns mouse position.
If you specify NONE it will return *Point. (if you just want position)
*/
Vec2 *mouse( MB flag=MB::NONE );
