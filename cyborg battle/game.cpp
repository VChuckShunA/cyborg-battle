#include "game.h"

Game::Game() {
	string resPath = getResourcePath();
	backgroundImage = loadTexture(resPath + "map.png", Globals::renderer);
	splashImage = loadTexture(resPath + "cyborgtitle.png", Globals::renderer);
	overlayImage = loadTexture(resPath + "overlay.png", Globals::renderer);

	splashShowing = true;
	overlayTimer = 2;

	//Setup camera
	Globals::camera.x = 0;
	Globals::camera.y = 0;
	Globals::camera.w = Globals::ScreenWidth;
	Globals::camera.h = Globals::ScreenHeight;

	//load up sounds
	SoundManager::soundManager.loadSound("hit", resPath + "Randomize2.wav");
	SoundManager::soundManager.loadSound("enemyHit", resPath + "Hit_Hurt9.wav");
	SoundManager::soundManager.loadSound("swing", resPath + "Randomize21.wav");
	SoundManager::soundManager.loadSound("dash", resPath + "dash.wav");
	SoundManager::soundManager.loadSound("growl", resPath + "Randomize34.wav");
	SoundManager::soundManager.loadSound("enemyDie", resPath + "Randomize41.wav");
	//Enemy Boss sounds
	SoundManager::soundManager.loadSound("crash", resPath + "cash.wav");
	SoundManager::soundManager.loadSound("smash", resPath + "smash.wav");
	SoundManager::soundManager.loadSound("shoot", resPath + "shoot2.wav");
	SoundManager::soundManager.loadSound("laugh", resPath + "laugh2.wav");

	song = Mix_LoadMUS(string(resPath+"Fatal Theory.wav").c_str()); //Song by Ryan Beveridge
	if (song != NULL)
		Mix_PlayMusic(song, -1);

	//holds a list of group types. This is list describes the types of groups of data our frames can have
	list<DataGroupType> dataGroupTypes;

	//so what data can a frame have?
	//collisionBoxes (althuough we have hardcoded the collision boxes)
	DataGroupType colBoxType;
	colBoxType.groupName = "collisionBox";
	colBoxType.dataType = DataGroupType::DATATYPE_BOX;

	//hitboxes
	DataGroupType hitBoxType;
	hitBoxType.groupName = "hitBox";
	hitBoxType.dataType= DataGroupType::DATATYPE_BOX;

	//damage
	DataGroupType dmgType;
	dmgType.groupName = "damage";
	dmgType.dataType = DataGroupType::DATATYPE_NUMBER;

	//add all of these dataTypes to the list
	dataGroupTypes.push_back(colBoxType);
	dataGroupTypes.push_back(hitBoxType);
	dataGroupTypes.push_back(dmgType);

	heroAnimSet = new AnimationSet();
	heroAnimSet->loadAnimationSet("udemyCyborg.fdset", dataGroupTypes, true, 0,true);

	globAnimSet = new AnimationSet();
	globAnimSet->loadAnimationSet("glob.fdset", dataGroupTypes, true, 0, true);

	grobAnimSet = new AnimationSet();
	grobAnimSet->loadAnimationSet("grob.fdset", dataGroupTypes, true, 0, true);

	roundKingAnimSet = new AnimationSet();
	roundKingAnimSet->loadAnimationSet("roundKing.fdset", dataGroupTypes, true, 0, true);

	bulletAnimSet = new AnimationSet();
	bulletAnimSet->loadAnimationSet("bullet.fdset", dataGroupTypes, true, 0, true);

	wallAnimSet = new AnimationSet();
	wallAnimSet->loadAnimationSet("wall.fdset", dataGroupTypes);

	//build hero entity
	hero = new Hero(heroAnimSet);
	hero->invincibleTimer = 0;
	hero->x = Globals::ScreenWidth / 2;
	hero->y = Globals::ScreenHeight / 2;
	//tells keyboard input to manage hero
	heroInput.hero = hero;
	//add hero to the entity list
	Entity::entities.push_back(hero);

	//Get camera to follow hero
	camController.target = hero;


	int tileSize = 32;
	//build all the walls for this game
	//first, build walls on the tpo and bottom the screen
	for (int i = 0; i < Globals::ScreenWidth / tileSize; i++) {
		//fills in top row 
		Wall* newWall = new Wall(wallAnimSet);
		newWall->x = i * tileSize + tileSize / 2;
		newWall->y = tileSize / 2;
		walls.push_back(newWall);
		Entity::entities.push_back(newWall);

		//re using pointer to create bottom row
		newWall = new Wall(wallAnimSet);
		newWall->x = i * tileSize + tileSize / 2;
		newWall->y=Globals::ScreenHeight- tileSize / 2;
		walls.push_back(newWall);
		Entity::entities.push_back(newWall);
	}
	//now the sides
	for (int i = 0; i < Globals::ScreenHeight / tileSize; i++) {
		//fills in left colum 
		Wall* newWall = new Wall(wallAnimSet);
		newWall->x = tileSize / 2;
		newWall->y = i * tileSize + tileSize / 2;
		walls.push_back(newWall);
		Entity::entities.push_back(newWall);

		//re using pointer to create right column
		newWall = new Wall(wallAnimSet);
		newWall->x = Globals::ScreenWidth - tileSize / 2;
		newWall->y = i * tileSize + tileSize / 2;
		walls.push_back(newWall);
		Entity::entities.push_back(newWall);
	}
	buildBossNext = false;
	bossActive = false;

	//setup hpBar's x's and y to be centered
	hpBar.x = Globals::ScreenWidth / 2.0f - (hpBar.barWidth / 2.0f);//centered horizonatally
	hpBar.y = Globals::ScreenHeight - hpBar.barHeight - 20;//20 pixels off the bottom of the screen
}
Game::~Game() {
	cleanup(backgroundImage);
	cleanup(splashImage);
	cleanup(overlayImage);

	Mix_PauseMusic();
	Mix_FreeMusic(song);

	if (scoreTexture != NULL)//if used, then cleanup
		cleanup(scoreTexture);

	Entity::removeAllFromList(&Entity::entities, false);

	delete heroAnimSet;
	delete globAnimSet;
	delete grobAnimSet;
	delete wallAnimSet;
	delete roundKingAnimSet;
	delete bulletAnimSet;

	delete hero;

	//delete all of the wall entities
	Entity::removeAllFromList(&walls, true);
	Entity::removeAllFromList(&enemies, true);
}

void Game::update() {
	//enemy related
	int enemiesToBuild = 2;
	int enemiesBuilt = 0;
	float enemyBuildTimer = 1;

	bool quit = false;
	
	SDL_Event e;
	//setup my time controller before the game starts
	TimeController::timeController.reset();
	//game loop
	while (!quit) {
		TimeController::timeController.updateTime();

		Entity::removeInactiveEntitiesFromList(&Entity::entities, false);
		//remove/delete enemies in the enemyList who are dead/inactive
		Entity::removeInactiveEntitiesFromList(&enemies, true);

		//check for any events that might have happened
		while (SDL_PollEvent(&e)) {
			//close the window
			if (e.type == SDL_QUIT)
				quit = true;
			//if keydown event
			if (e.type == SDL_KEYDOWN) {
				//switch case on which button was pressed
				switch (e.key.keysym.scancode)
				{
				case SDL_SCANCODE_ESCAPE: //esc key
					quit = true;
					break;
				case SDL_SCANCODE_SPACE: //space key
					if (splashShowing)
						splashShowing = false;
					if (overlayTimer <= 0 && hero->hp < 1) {
						//cleanup and restart the game
						enemiesToBuild = 2;
						enemiesBuilt = 0;
						enemyBuildTimer = 3;
						overlayTimer = 2;
						enemyWavesTillBoss = 3;
						bossActive = false;
						buildBossNext = false;
						//make HpBar point to no entities health
						hpBar.entity == NULL;

						RoundKing::roundKingsKilled = 0;
						Glob::globsKilled = 0;
						Grob::grobsKilled = 0;

						if (scoreTexture != NULL) {
							cleanup(scoreTexture);
							scoreTexture = NULL;
						}

						//remove all existing enemies
						for (list<Entity*>::iterator enemy = enemies.begin(); enemy != enemies.end(); enemy++) {
							(*enemy)->active = false;
						}
						hero->revive();
					}
					break;
				}
			}
			heroInput.update(&e);//updating hero inputs
		}
		//make our overlay timer tick down
		if (hero->hp < 1 && overlayTimer>0) {
			overlayTimer -= TimeController::timeController.dT;
		}
		//update all entities
		for (list<Entity*>::iterator entity = Entity::entities.begin(); entity != Entity::entities.end(); entity++) {
			//remember how awesome polymorphism is?
			//update all entities in the game world at once
			(*entity)->update();
		}

		//spawn enemies
		if (hero->hp > 0 && !splashShowing) {
			if (enemiesToBuild == enemiesBuilt && enemies.size()<=0) {
				enemiesToBuild = enemiesToBuild + 2;
				enemiesBuilt = 0;
				enemyBuildTimer = 4;
				enemyWavesTillBoss--;

				if (enemyWavesTillBoss <= 0) {
					buildBossNext = true;
				}
			 }
			enemyBuildTimer -= TimeController::timeController.dT;
			//if no Bosses on the prowl, check to see if we should build some jerks
			if (!buildBossNext && !bossActive && enemyBuildTimer <= 0 && enemiesBuilt < enemiesToBuild && enemies.size() < 10) {
				Glob* enemy = new Glob(globAnimSet);
				//set enemies' position to somewhere random within the arena's open space
				enemy->x = getRandomNumber(Globals::ScreenWidth - (2 * 32) - 32) + 32 + 16; //dumb calculation done to find a spot not too close to the walls
				enemy->y = getRandomNumber(Globals::ScreenHeight - (2 * 32) - 32) + 32 + 16;
				enemy->invincibleTimer = 0.1;

				enemies.push_back(enemy);
				Entity::entities.push_back(enemy);

				enemiesBuilt++;
				enemyBuildTimer = 1;

				//grob Spawn
				/*if (enemiesBuilt % 5 == 0)
				{
					Grob* grob = new Grob(grobAnimSet);
					grob->x = getRandomNumber(Globals::ScreenWidth - (2 * 32) - 32) + 32 + 16; //random x value between our walls
					grob->y = getRandomNumber(Globals::ScreenHeight - (2 * 32) - 32) + 32 + 16; //random x value between our walls
					grob->invincibleTimer = 0.01;

					enemies.push_back(grob);
					Entity::entities.push_back(grob);
				}*/
			}

			//FOR THE BOSS
			if (buildBossNext && enemyBuildTimer <=0 && enemies.size()==0) {
				RoundKing *round = new RoundKing(roundKingAnimSet,bulletAnimSet);
				round->invincibleTimer = 0.1;
				enemies.push_back(round);
				Entity::entities.push_back(round);

				//make hpBar point to Boss
				hpBar.entity = round;

				bossActive = true;
				buildBossNext = false;
				enemyWavesTillBoss = 3;
			}
			//if there was a boss, but he's dead now, go back to spawning normal enemies till the next boss
			if (bossActive && enemies.size() == 0)
			{
				bossActive = false;
				buildBossNext = false;
				enemiesBuilt = 0;
				enemiesToBuild = 2;

				//when boss dead, make sure hpBar doesn't reference him anymore
				hpBar.entity = NULL;
			}
		}


		//update camera position
		camController.update();

		//draw all entities
		draw();
	}
}
void Game::draw() {
	//clear the screen
	SDL_SetRenderDrawColor(Globals::renderer, 145, 133, 129, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(Globals::renderer);
	if (splashShowing) {

		renderTexture(splashImage, Globals::renderer, 0 , 0 );
	}
	else {
		//draw the background
		renderTexture(backgroundImage, Globals::renderer, 0 - Globals::camera.x, 0 - Globals::camera.y); //remove the camera substraction if you want the background to go forever

		//sort all entities based on y(depth)
		Entity::entities.sort(Entity::EntityCompare);
		//draw all of the entities 
		for (list<Entity*>::iterator entity = Entity::entities.begin(); entity != Entity::entities.end(); entity++) {
			(*entity)->draw();
		}

		//Draw UI stuff
		hpBar.draw();

		if (overlayTimer <= 0 && hero->hp < 1) {
			renderTexture(overlayImage, Globals::renderer, 0, 0);
			if (scoreTexture == NULL) {
				//generate score text
				SDL_Color colour = { 255,255,255,255 };//white

				stringstream ss;
				ss << "Enemies dispatched: " << Glob::globsKilled + Grob::grobsKilled+RoundKing::roundKingsKilled;

				string resPath = getResourcePath();
				scoreTexture = renderText(ss.str(), resPath + "vermin_vibes_1989.ttf",colour,30,Globals::renderer);
			}
			renderTexture(scoreTexture, Globals::renderer, 20, 50);
		}
	}

	//after we're done drawing/rendering, show it to the screen
	SDL_RenderPresent(Globals::renderer);
}