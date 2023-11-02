#ifndef TIMECONTROLLER
#define TIMECONTROLLER

#include <iostream>
#include "SDL.h"

class TimeController {
public:
	//reference values
	static const int PLAY_STATE, PAUSE_STATE;

	int timeState;
	Uint32 lastUpdate; //Last time we shcekd how many ticks we were up to
	float dT; //delta time, in seconds (1=1seconds,0.5 = half a second). Time since the last frame was rendered onto the screen

	TimeController();
	void updateTime();//update lastUpdate and dT
	void pause();
	void resume();
	void reset();

	static TimeController timeController;
};

#endif // !TIMECONTROLLER
