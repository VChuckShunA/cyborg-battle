#ifndef FRAME
#define FRAME

#include <iostream>
#include <list>
#include <sstream>
#include "SDL.h"
#include "drawing_functions.h"
#include "globals.h"
#include "groupBuilder.h"

using namespace std;

class Frame {
public:
	int frameNumber; // or frame index
	SDL_Rect clip; // its the region on the spritesheet where this frame
	float  duration; //how long does the frame run for
	SDL_Point offSet; //pivot point to help align frames in an animation

	list<Group*> frameData; //holds groups of additional data for the frame

	void Draw(SDL_Texture* spriteSheet,float x, float y);

	void loadFrame(ifstream& file, list<DataGroupType>& groupTypes);

	
};

#endif