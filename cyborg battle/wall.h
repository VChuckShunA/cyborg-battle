#ifndef WALL
#define WALL

#include "entity.h"

class Wall : public Entity {
public:
	Wall(AnimationSet* animSet);
	void update();
	void changeAnimation(int newState, bool resetFrameToBegining);
	void updateCollision() {
		//Walls don't do anything in this game
	}
};

#endif
