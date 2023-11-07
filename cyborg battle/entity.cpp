#include "entity.h"

const int Entity::DIR_UP = 0, Entity::DIR_DOWN = 1, Entity::DIR_LEFT = 2, Entity::DIR_RIGHT = 3, Entity::DIR_NONE = -1;

//VIRTUAL FUNCTIONS
void Entity::update() {}//override me to do something useful
void Entity::draw() {
	//override me to do something else or more specific done
	//draws current frame
	if (currentFrame != NULL && active) {
		currentFrame->Draw(animSet->spriteSheet, x, y);
	}
	//draw collisionBox
	if (solid && Globals::debugging) {
		//sets the current drawing colour (Doesn't affect textures and what not)
		SDL_SetRenderDrawColor(Globals::renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawRect(Globals::renderer, &collisionBox);
	}
}

void Entity::move(float angle) {
	moving = true;
	moveSpeed = moveSpeedMax;
	this->angle = angle;

	int newDirection = angleToDirection(angle);
	//if direction changed, update current animation
	if (direction != newDirection)
	{
		direction = newDirection;
		changeAnimation(state,false);
	}
}
void Entity::updateMovement(){
	updateCollisionBox();
	//store collision Box before we move
	lastCollisionBox = collisionBox;

	//reset total moves
	totalXMove = 0;
	totalYMove = 0;

	if (moveSpeed > 0)
	{
		//works out move distance using speed, dt(time since last loop) and multiplies by lerp
		float moveDist = moveSpeed * (TimeController::timeController.dT) * moveLerp;
		if (moveDist > 0)
		{
			//xmove= distance * cos(angle in radians)
			float xMove = moveDist * (cos(angle * Globals::PI / 180));
			//ymove= distance * cos(angle in radians)
			float yMove = moveDist * (sin(angle * Globals::PI / 180));

			x += xMove;
			y += yMove;

			totalXMove = xMove;
			totalYMove = yMove;

			if (!moving) {
				moveSpeed -= moveDist;
			}
		}
	}
	//sliding around
	if (slideAmount > 0) {
		float slideDist = slideAmount * TimeController::timeController.dT * moveLerp;
		if (slideDist > 0) {
			float xMove = slideDist * (cos(slideAngle * Globals::PI / 180));
			float yMove = slideDist * (sin(slideAngle * Globals::PI / 180));

			x += xMove;
			y += yMove;

			totalXMove += xMove;
			totalYMove += yMove;
			slideAmount -= slideDist;

		}
		else {
			slideAmount = 0;
		}
	}

	//now that we've moved, move the collision box upto where we are now
	updateCollisionBox();
	//to help with collision checking, union collisionBox with lastCollisionBox
	SDL_UnionRect(&collisionBox, &lastCollisionBox, &collisionBox);

}
void Entity::updateCollisionBox(){
	collisionBox.x = x - collisionBox.w / 2;
	collisionBox.y = y + collisionBoxYOffset;
	collisionBox.w = collisionBoxW;
	collisionBox.h = collisionBoxH;
}

//how we bump into stuff in the world
void Entity::updateCollisions() {
	if (active && collideWithSolids && (moveSpeed > 0 || slideAmount > 0))
	{
		//list of potential collisions
		list<Entity*> collisions;

		//list<Entity*>::iterator entity=... // alternative code
		for (auto entity = Entity::entities.begin(); entity != Entity::entities.end(); entity++) // list of entities we've collided with
		{
			//if we collide with this entity with our currently unioned collision box, add it to the list.
			if((*entity)->active // if... this entity is active
				&& (*entity)->type!= this->type //is not my type
				&& (*entity)->solid // is solid
				&& Entity::checkCollision(collisionBox, (*entity)->collisionBox))// We ARE colliding with it
			{
			// Add it to the list
				collisions.push_back(*entity);
			}
		}

		// if we have a list of potential entities that we may hit, then lets check them properly to do collision resolution
		if (collisions.size() > 0) {
			updateCollisionBox();

			//multisample check for collisions form where we started to where we are planning to go to

			//first we are going to find the sample distance we shoudld travel between
			float boxTravelSize = 0;
			if (collisionBox.h < collisionBox.w)
			{
				boxTravelSize = collisionBox.w / 4;
			}
			else
			{
				boxTravelSize = collisionBox.h / 4;
			}

			//use samplebox to check for collisions from start point to end point, moving at boxTravelSize each sample
			SDL_Rect sampleBox = lastCollisionBox;
			float movementAngle = Entity::angleBetweenTwoRects(lastCollisionBox, collisionBox);

			bool foundCollision = false;
			while (!foundCollision)
			{
				//check samplebox for collisions where it is now
				SDL_Rect intersection;
				for (auto entity = collisions.begin(); entity != collisions.end(); entity++) {
					if (SDL_IntersectRect(&sampleBox, &(*entity)->collisionBox, &intersection))
					{
						foundCollision = true;
						moveSpeed = 0;
						moving = false;
						slideAngle = angleBetweenTwoEntities((*entity),this); //The entity will slide backwards at this angle

						//currently intersecting an entity, now we need to do collision resolution
						if (intersection.w < intersection.h)
						{
							if (lastCollisionBox.x + lastCollisionBox.w / 2 < (*entity)->collisionBox.x + (*entity)->collisionBox.w / 2) //pushing from the left?
							{
								sampleBox.x -= intersection.w;//started on left, so we move left out of collision
							}
							else {
								sampleBox.x += intersection.w;//started on right, so we move right out of collision
							}
						}
						else {
							if (lastCollisionBox.y + lastCollisionBox.h / 2 < (*entity)->collisionBox.y + (*entity)->collisionBox.h / 2)
							{
								sampleBox.y -= intersection.h;//started above,move up
							}
							else {
								sampleBox.y += intersection.h;//otherwise, started below
							}
						}
					}
				}

				//if collisions found or sample box is at same place as collisionBox, exit loop
				if (foundCollision || (sampleBox.x == collisionBox.x && sampleBox.y == collisionBox.y))
					break;
				//move sample box up to check the next spot
				if (distanceBetweenTwoRects(sampleBox, collisionBox) > boxTravelSize) {
					float xMove = boxTravelSize * (cos(movementAngle*Globals::PI/180));
					float yMove = boxTravelSize * (sin(movementAngle * Globals::PI / 180));

					sampleBox.x += xMove;
					sampleBox.y += yMove;
				}
				else {
					sampleBox = collisionBox;
				}
			}
			if (foundCollision)
			{
				//move our  entity to where the sampleBox ended up
				slideAmount = slideAmount / 2;
				x = sampleBox.x + sampleBox.w / 2;
				y = sampleBox.y - collisionBoxYOffset;
			}
			updateCollisionBox();
		}
	}
} 

//HELP FUNCTIONS
static float Entity::distanceBetweenTwoRects(SDL_Rect& r1, SDL_Rect& r2);
static float Entity::distanceBetweenTwoEntities(Entity* e1, Entity* e2);
static float Entity::angleBetweenTwoEntities(Entity* e1, Entity* e2);
static bool Entity::checkCollision(SDL_Rect cbox1, SDL_Rect cbox2);
static int Entity::angleToDirection(float angle);
static float Entity::angleBetweenTwoPoints(float cx1, float cy1, float cx2, float cy2);
static float Entity::angleBetweenTwoRects(SDL_Rect& r1, SDL_Rect& r2);

//global entities list I can refer to at any time
static list<Entity*> entities;
static bool Entity::EntityCompare(const Entity* const& a, const Entity* const& b);// compare 2 entities in a list to help sorting (sorts based on y value)
static void Entity::removeInactiveEntitiesFromList(list<Entity*>* entityList, bool deleteEntities);
static void Entity::removeAllFromList(list<Entity*>* entityList, bool deleteEntities);;