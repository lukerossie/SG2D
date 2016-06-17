#include "../include/input.hpp"
#include <map>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
std::map<SDL_Keycode, bool> keyState;
SDL_mutex *mutex = SDL_CreateMutex();

std::map<char, bool> mouseState;
Vec2 mousePos;//internal use
/*
mouse() will either return null or a pointer to this mouseCopy,
copied respecting mutex.
*/
Vec2 mouseCopy;
Vec2 *mousePointer;
Vec2 mouseScrollVal;
}
void *onWindowEvent=0;
Vec2 mouseScroll;
std::function<void(int)> keyDownEvent;
std::function<void(int)> keyUpEvent;

void poll()
{
    SDL_Event event;
	mouseScroll.x=0;
	mouseScroll.y=0;
    while(SDL_PollEvent( &event ))
	{
		if( event.type == SDL_QUIT ) exit( 0 );//if they try to close the window we just quit immediately

		if( event.key.type == SDL_KEYDOWN )
		{
			keyState[event.key.keysym.sym] = true;
			if(keyDownEvent)keyDownEvent(event.key.keysym.sym);
		}
		if( event.key.type == SDL_KEYUP )
		{
			keyState[event.key.keysym.sym] = false;
			if(keyUpEvent)keyUpEvent(event.key.keysym.sym);
		}

		if( event.button.type == SDL_MOUSEBUTTONDOWN )
		{
			mouseState[event.button.button] = true;
			mousePos.x = event.button.x;
			mousePos.y = event.button.y;
		}
		if( event.button.type == SDL_MOUSEBUTTONUP )
		{
			mouseState[event.button.button] = false;
			mousePos.x = event.button.x;
			mousePos.y = event.button.y;
		}

		if( event.button.type == SDL_MOUSEMOTION )
		{
			mousePos.x = event.motion.x;
			mousePos.y = event.motion.y;
		}
		if( event.wheel.type == SDL_MOUSEWHEEL )
		{
			mouseScrollVal.x = -event.wheel.x;
			mouseScrollVal.y = -event.wheel.y;
			mouseScroll=mouseScrollVal;
		}

		if( event.type == SDL_WINDOWEVENT )
		{
			if(onWindowEvent)
			{
				auto func=(void (*)(SDL_Event*))onWindowEvent;
				func(&event);
			}
		}
	}
}

bool key( SDL_Keycode whichKey )
{
	if( whichKey > 1073742106 || whichKey < 0 ) return false;
	bool value = keyState[whichKey];
	return value;
}
Vec2 *mouse( MB flag )
{
	SDL_LockMutex( mutex );
	mouseCopy = mousePos;
	SDL_UnlockMutex( mutex );
	mousePointer = &mouseCopy;

	if(flag == MB::LEFT)
	{
		if(mouseState[SDL_BUTTON_LEFT])
		{
			return mousePointer;
		}
		else
		{
			return 0;
		}
	}
	else if(flag == MB::RIGHT)
	{
		if(mouseState[SDL_BUTTON_RIGHT])
		{
			return mousePointer;
		}
		else
		{
			return 0;
		}
	}
	else if(flag == MB::MIDDLE)
	{
		if(mouseState[SDL_BUTTON_MIDDLE])
		{
			return mousePointer;
		}
		else
		{
			return 0;
		}
	}
	else if(flag == MB::NONE)
	{
		return mousePointer;
	}
	return 0;
}

#ifdef SG2D_TEST
void input_test()
{
}
#endif
