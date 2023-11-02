#include "globals.h"


 const float Globals::PI=3.14159;

//useful for me as a dev
bool Globals::debugging=true;

//sdl related 
int Globals::ScreenWidth=640, Globals::ScreenHeight=352, Globals::ScreenScale=2;
SDL_Renderer* Globals::renderer=NULL;