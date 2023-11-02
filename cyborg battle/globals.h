#ifndef GLOBALS
#define GLOBALS

#include <string>
#include <iostream>
#include <SDL.h>
#include "randomNumber.h"

class Globals {
public:
	//math helpers
	static const float PI;

	//useful for me as a dev
	static bool debugging;

	//sdl related 
	static int ScreenWidth, ScreenHeight, ScreenScale;
	static SDL_Renderer* renderer;

};
#endif // !GLOBALS
