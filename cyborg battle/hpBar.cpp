#include "hpBar.h"

void HPBar::draw() {
	if (entity != NULL) {
		SDL_SetRenderDrawColor(Globals::renderer, 240, 51, 159, SDL_ALPHA_OPAQUE);
		//draw outside of hp bar
		SDL_Rect barContainer = { x,y,barWidth,barHeight };
		SDL_RenderDrawRect(Globals::renderer, &barContainer);

		//perecentage of entities health remaining
		float hpPercent = entity->hp / (entity->hpMax*1.0f); //1.0f upgrade math into float division so we don't lose decimals
			//example 50/100 - 0.5 but in integer division, removes decimal, so we get 0


		SDL_Rect hpRect = { x + 2,y + 2,(barWidth - 4) * hpPercent,barHeight - 4 };//trimming 4 pixels off to make up for the size of the rect box

		SDL_RenderFillRect(Globals::renderer, &hpRect);
	}
}