#include "entity.h"

const int Entity::DIR_UP = 0, Entity::DIR_DOWN = 1, Entity::DIR_LEFT = 2, Entity::DIR_RIGHT = 3, Entity::DIR_NONE = -1;

//VIRTUAL FUNCTIONS
void Entity::update() {}//override me to do something useful
void Entity::draw() {
	//override me to do something else or more specific done
	//draws current frame
	if (currentFrame != NULL && active) {
		currentFrame->Draw(animSet->spriteSheet, x-Globals::camera.x, y-Globals::camera.y);
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

		//BROAD PHASE
		//list<Entity*>::iterator entity=... // alternative code
		for (auto entity = Entity::entities.begin(); entity != Entity::entities.end(); entity++) // list of entities we've collided with
		{
			//if we collide with this entity with our currently unioned collision box, add it to the list.
			if((*entity)->active // if... this entity is active
				//&& (*entity)->type!= this->type //is not my type// add this line back in if things of the same type can walk through each other
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

			float collisionTime = 1;//1 means no collisions, anything less, e.g 0.234, means we collided part of the way of our movement
			float normalX, normalY; //to help work out which side we crash into stuff

			//our collisionBox before we tried to move
			SDL_Rect startingCollisionBox = lastCollisionBox;

			//loop through the entities that are in our short list from broadphase
			for (auto entity = collisions.begin(); entity != collisions.end(); entity++) {
				//temporary variables for normal x and y and also temp collisionTime
				float tmpNormalX, tmpNormalY;
				float tmpCollisionTime = SweptAABB(startingCollisionBox, totalXMove, totalYMove,(*entity)->collisionBox,tmpNormalX,tmpNormalY);

				//if this tmpCollision Time is less than last collision time, use it instead
				if (tmpCollisionTime < collisionTime) {
					collisionTime = tmpCollisionTime;
					normalX = tmpNormalX;
					normalY = tmpNormalY;
				}
			}

			//if there was a collision, lets slide off of it
			if (collisionTime < 1.0f) {
				//if we die on solids, run the crash function
				if (dieOnSolids) {
					crashOntoSolid();
				}

				//Move our collisionBox position up to where we collided
				startingCollisionBox.x += totalXMove * collisionTime;
				startingCollisionBox.y += totalYMove * collisionTime;

				//how much move time was remaining
				float remainingTime = 1.0f - collisionTime;
				//update entities x and y to where we bumped into other entity
				x = startingCollisionBox.x + startingCollisionBox.w / 2;
				y = startingCollisionBox.y - collisionBoxYOffset;
				//COLLISION RESPONSE: SLIDE
				//work out dotProduct using remainingTime
				float dotProd = (totalXMove * totalYMove * normalX) * remainingTime;
				totalXMove = dotProd * normalX;
				totalYMove = dotProd * normalY;
				x += totalXMove;
				y += totalYMove;
				//store collisionBox at this point
				lastCollisionBox = startingCollisionBox;
				//update our entities main collisionBox
				updateCollisionBox();

				//BECAUSE OUR SLIDING MAY HAVE BUMPED INTO OTHER OBJECT, BETTER RUN THE FUNCTION AGAIN
				updateCollisions();
			}
			/* OLD COLLISION
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
			float collisionMoveX = sampleBox.x, collisionMoveY = sampleBox.y;
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
					movementAngle = Entity::angleBetweenTwoRects(sampleBox, collisionBox);//course correction
					float xMove = boxTravelSize * (cos(movementAngle*Globals::PI/180)); 
					float yMove = boxTravelSize * (sin(movementAngle * Globals::PI / 180));

					//Fix for float values below 1 e.g 0.71 being truncated to sampleBox.x as thats an it
					collisionMoveX += xMove;
					collisionMoveY += yMove;
					sampleBox.x = collisionMoveX;
					sampleBox.y = collisionMoveY;
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
			updateCollisionBox();*/
		}
	}
} 

//HELP FUNCTIONS

//return type: gives 0-1 depending on where collision is. 1 means, no collisions, 0 means collide immediately and 0.5 is half way.etc
	//params: movingBox is entity being checked
	//		vx and vy are the velocities our moving box is moving at
	//		otherbox is some other entitie's collision box we may collide with
	//		normalX and normalY let us know which side of the the otherBox we collided with, These are pass by reference	

float Entity::SweptAABB(SDL_Rect movingBox, float vx, float vy, SDL_Rect otherBox, float& normalX, float& normalY) {
	float xInvEntry, yInvEntry;
	float xInvExit, yInvExit;

	//find the distance between the opjects on the near end and far of both x and y
	if (vx > 0.0f) {
		xInvEntry = otherBox.x - (movingBox.x + movingBox.w);
		xInvExit = (otherBox.x + otherBox.w) - movingBox.x;
	}
	else {
		xInvEntry = (otherBox.x + otherBox.w) - movingBox.x;
		xInvExit= otherBox.x - (movingBox.x + movingBox.w);
	}

	if (vy > 0.0f) {
		yInvEntry = otherBox.y - (movingBox.y + movingBox.h);
		yInvExit = (otherBox.y + otherBox.h) - movingBox.y;
	}
	else {
		yInvEntry = (otherBox.y + otherBox.h) - movingBox.y;
		yInvExit = otherBox.y - (movingBox.y + movingBox.h);
	}

	//find time of collision and time of leacing for each axis(if statement is to rpvevent dividing by zero)
	float xEntry, yEntry;
	float xExit, yExit;

	if (vx == 0.0f) {
		xEntry = -std::numeric_limits<float>::infinity();
		xExit = std::numeric_limits<float>::infinity();
	}
	else {
		xEntry = xInvEntry / vx;
		xExit = xInvEntry / vx;
	}

	if (vy == 0.0f) {
		yEntry = -std::numeric_limits<float>::infinity();
		yExit = std::numeric_limits<float>::infinity();
	}
	else {
		yEntry = yInvEntry / vy;
		yExit = yInvEntry / vy;
	}

	//find the earliest/latest times of collision
	float entryTime = std::max(xEntry, yEntry);
	float exitTime = std::max(xExit, yExit);

	//if there was NO collision
	if (entryTime > exitTime || xEntry < 0.0f && yEntry < 0.0f || xEntry>1.0f||yEntry>1.0f) {
		normalX = 0.0f;//no x normal
		normalY = 0.0f; //no y normal
		return 1.0f;//1 means we moved the full distance,no collisions
	}
	else {
		//there was a collision

		//we should work out which sides/normals of the otherBox we collided with
		if (xEntry > yEntry) { //assume we're hit on the xAxis
			if (xInvEntry < 0.0f) {
				normalX = 1; //hit right hand side
				normalY = 0; //not hit tpo or bottom of otherBox
			}
			else {
				normalX = -1; //hit left hand side
				normalY = 0;//not hit tpo or bottom of otherBox
			}
		}
		else {
			//assume we hit otherBox on the yaxis
			if (yInvEntry < 0.0f) {
				normalX = 0; //did not hit right hand side
				normalY = 1; //hit top or bottom of otherBox
			}
			else {
				normalX = 0; //did not hit left hand side
				normalY = -1;//hit top or bottom of otherBox
			}
		}
		//return the time of collision(0-1 e.g 0.2323432)
		return entryTime;
	}
}
float Entity::distanceBetweenTwoRects(SDL_Rect& r1, SDL_Rect& r2) {
	SDL_Point e1, e2;
	e1.x = r1.x + r1.w / 2;
	e1.y = r1.y + r1.h / 2;

	e2.x = r2.x + r2.w / 2;
	e2.y = r2.y + r2.h / 2;

	float d = abs(sqrt(pow(e2.x - e1.x, 2) + pow(e2.y - e1.y, 2)));
	return d;
}
float Entity::distanceBetweenTwoEntities(Entity* e1, Entity* e2) {
	float d = abs(sqrt(pow(e2->x - e1->x, 2) + pow(e2->y - e1->y, 2)));
	return d;
}

float Entity::distanceBetweenTwoPoints(float cx1, float cy1, float cx2, float cy2) {
	float d = abs(sqrt(pow(cx2 - cx1, 2) + pow(cy2 - cy1, 2)));
	return d;
}

float Entity::angleBetweenTwoEntities(Entity* e1, Entity* e2) {
	float dx, dy;
	float x1 = e1->x, y1 = e1->y, x2 = e2->x, y2 = e2->y;

	dx = x2 - x1;
	dy = y2 - y1;

	return atan2(dy, dx) * 180 / Globals::PI;
}
bool Entity::checkCollision(SDL_Rect cbox1, SDL_Rect cbox2) {
	SDL_Rect intersection;
	if (SDL_IntersectRect(&cbox1, &cbox2, &intersection)) {
		return true;
	}
	//if a rectangle is in another rectangle
		//do it here
	return false;
}
int Entity::angleToDirection(float angle) {
	if ((angle >= 0 && angle <= 45) || angle >= 315 && angle <= 360)
	{
		return DIR_RIGHT;
	}
	else if (angle >= 45 && angle <= 135) {
		return DIR_DOWN;
	}
	else if (angle >= 135 && angle <= 225) {
		return DIR_LEFT;
	}
	else
		return DIR_UP;
}
float Entity::angleBetweenTwoPoints(float cx1, float cy1, float cx2, float cy2) {
	float dx = cx2 - cx1;
	float dy = cy2 - cy1;

	return atan2(dy,dx)* 180/ Globals::PI;
}
float Entity::angleBetweenTwoRects(SDL_Rect& r1, SDL_Rect& r2) {
	float cx1 = r1.x + (r1.w / 2);
	float cy1 = r1.y + (r1.h / 2);

	float cx2 = r2.x + (r2.w / 2);
	float cy2 = r2.y + (r2.h / 2);

	return angleBetweenTwoPoints(cx1,cy1,cx2,cy2);
}

//global entities list I can refer to at any time
list<Entity*> Entity::entities;
bool Entity::EntityCompare(const Entity* const& a, const Entity* const& b) {
	// compare 2 entities in a list to help sorting (sorts based on y value)
	if (a != 0 && b != 0)
	{
		return (a->y < b->y);
	}
	else {
		return false;
	}
}
void Entity::removeInactiveEntitiesFromList(list<Entity*>* entityList, bool deleteEntities) {
	for (auto entity = entityList->begin(); entity != entityList->end();)
	{
		//if entity is not active
		if (!(*entity)->active)
		{
			if (deleteEntities)
			{
				delete (*entity);
			}
			entity = entityList->erase(entity);
		}
		else
		{
			entity++;
		}
	}

}
void Entity::removeAllFromList(list<Entity*>* entityList, bool deleteEntities) {
	for (auto entity = entityList->begin(); entity != entityList->end();)
	{
			if (deleteEntities)
			{
				delete (*entity);
			}
			entity = entityList->erase(entity);
	
	}
}