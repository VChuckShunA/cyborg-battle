#include "keyboardInput.h"

KeyboardInput::KeyboardInput() {
	//hardcoded keyboard buttons
	UP = SDL_SCANCODE_UP;
	DOWN = SDL_SCANCODE_DOWN;
	LEFT = SDL_SCANCODE_LEFT;
	RIGHT = SDL_SCANCODE_RIGHT;
	SLASH = SDL_SCANCODE_Z;
	DASH = SDL_SCANCODE_X;
	//Bonus tasl : load scancodes up from a textfile. THis will be needed when changing controls
	//int test=82l
	//UP =(SDL_Scancode)test;
}
void KeyboardInput::update(SDL_Event* e) {
	//button presses
	if (e->type == SDL_KEYDOWN) {
		if (e->key.keysym.scancode == DASH) {
			hero->dash();
		}if (e->key.keysym.scancode == DASH) {
			hero->slash();
		}
	}

	//button holds
	//chekc for keys still behing held
	const Uint8* keyStates = SDL_GetKeyboardState(NULL);
	//if hero not able to move or no direction buttons are being held down, then stop moving(slide to a halt)
	if((hero->state !=Hero::HERO_STATE_MOVE &&  hero->state != Hero::HERO_STATE_IDLE) // if we're not moving and not idle OR
		|| (!keyStates[UP] && !keyStates[DOWN] && !keyStates[LEFT] && !keyStates[RIGHT])) // if none of the movement keys are pressed
	{
		hero->moving = false;
	}
	else {
		//moving upward
		if (keyStates[UP]) {
			//up right and up left
			if (keyStates[RIGHT])
				hero->move(270 + 45);
			else if (keyStates[LEFT])
				hero->move(270 - 45);
			else
				hero->move(270);
		}
		//moving downwards
		if (keyStates[DOWN]) {
			//up right and up left
			if (keyStates[RIGHT])
				hero->move(90 + 45);
			else if (keyStates[LEFT])
				hero->move(90 - 45);
			else
				hero->move(90);
		}

		//moving left
		if (!keyStates[UP] && !keyStates[DOWN] && !keyStates[RIGHT] && keyStates[LEFT])
			hero->move(180);
		//moving right
		if (!keyStates[UP] && !keyStates[DOWN] && keyStates[RIGHT] && !keyStates[LEFT])
			hero->move(0);

	}
}