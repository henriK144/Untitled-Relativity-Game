/* "Untitled Relativity Game". Originally made as the Personal Physics Project (PPP) for SPH4UE, By Henri K. 
A 2D platformer where the player must switch frames of reference from a train to a camera pointed at the train,
and exploit the fixed speed of light to induce time dilation, length contraction, and the relativity of simultaneity 
to reach the end of each level. 
Controls and other information are displayed in-game. 
The implementation of the basic sprite-displaying system is in entity.cpp and renderwindow.cpp, while the physics engine implementation is in body.cpp. 
The relativistic effects are governed by the FrameOfReference and lorentzFactor structs. */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <cassert>
#include <Windows.h>

#include "RenderWindow.hpp"
#include "Entity.hpp"
#include "Body.hpp"
#include "Surface.hpp"
#include "GameFuncs.hpp"

#define theBackground backgroundRenderQueue[i]
#define theBackgroundObj backgroundObjRenderQueue[i]
#define theObject objectRenderQueue[i]
#define theBody bodyRenderQueue[i]
#define theSurface surfaceRenderQueue[i]
#define elementX coordinates.first
#define elementY coordinates.second
#define repeat(n) for (int i = 1; i <= n; i++)

using std::string;
using std::abs;
using std::pair;
using std::vector;
using std::map;
using namespace gamefuncs;

const float SPEED_OF_LIGHT = 299792458.0; // in m/s.

struct FrameOfReference
{
	bool playerInFrame;
	float velocity; // With respect to Earth.
}; // It's an inertial frame of reference, of course.

float lorentzFactor(FrameOfReference s, FrameOfReference sPrime)
{
	float v = sPrime.velocity - s.velocity; // The velocity of the relative motion between the two frames.
	float c = SPEED_OF_LIGHT;
	return 1/sqrt(1 - (v*v)/(c*c)); // This formula is shown on several of the decorational blackboards in the game.
}

float dopplerFactor(float lorentz)
{
	float beta = 1 - (1/(lorentz*lorentz));
	return sqrt((1 - beta)/(1 + beta));
} // The factor governing the relativistic doppler effect, i.e. by which light frequencies are multiplied.

pair<Uint8,Uint8> dopplerShift(float dFactor) 
{
	pair<Uint8,Uint8> shiftAmounts;

	shiftAmounts.first = -99.6*dFactor + 139.84;
	shiftAmounts.second = -9.96*dFactor + 13.984; // These scale the redshift, so it's not too exaggerated.

	return shiftAmounts;
}

struct QueueSet
{
	vector<Entity>* queue1;
	vector<Entity>* queue2; 
	vector<Entity>* queue3; 
	vector<Body>* queue4; 
	vector<Surface>* queue5; 
	vector<float>* sizeQueue2; 
	vector<float>* sizeQueue3; 
	vector<float>* sizeQueue4; 
	vector<float>* sizeQueue5; 
	vector<bool>* hboxQueue; 
	vector<char>* animQueue;
}; // Organizes the several render queues. queue1=background, queue2=backgroundobject, queue3=object, queue4=body, queue5=surface.

struct LevelElement
{
	Entity* objptr;
	char type; // E=Entity, D=Decoration, B=Body, S=Surface.
	char animCode; // Dictates how (if at all) the object animates once rendered; this is primarily for obstacles. From a design perspective, having them animate brings attention to the fact that they can be interacted with or are potentially harmful. \0 (null char) = no animation, E=electrosphere, B=electro beam, F=initially-on flamethrower, G=initially-off flamethrower, M=missile, C=missile cannon, R=initially-off lightning, L=initially-on lightning,  K=key, H=health refill power-up. \0 returns false when passed as a bool.

	pair<float,float> coordinates; // first=x, second=y.
	pair<float,float> velocities; // first=x, second=y. Can only be nonzero if the element is a body or surface.
	float size;
	bool hitbox = false; // Only for Body instances.
}; // For preparing the objects which need to be put into levels using the Level struct.

struct Level
{
	float playerSize;
	bool floor, ceiling, leftWall, rightWall;
	bool doorLocked;

	pair<Entity,Entity> backgrounds;
	pair<float,float> playerLocation;
	pair<float,float> doorLocation;
	pair<float,float> cameraLocation; // If the camera is not in the level, this can simply be set to {-1000,-1000}.
	pair<float,float> simulCameraLocation; // If the camera is not in the level, this can simply be set to {-1000,-1000}.
	vector<LevelElement> elements;
}; // Contains all the information about a level's objects and initial conditions.

void loadLevel(Level l, Body& p, QueueSet& q, Entity& door, Entity& cam1, Entity& cam2) 
{
	clearEntities(q.queue5, q.sizeQueue5, q.animQueue);
	clearEntities(q.queue4, q.sizeQueue4, q.hboxQueue);
	clearEntities(q.queue3, q.sizeQueue3);
	clearEntities(q.queue2, q.sizeQueue2);
	q.queue1->clear();

	p.setCoords(l.playerLocation.first, l.playerLocation.second);
	q.queue1->push_back(l.backgrounds.first);
	q.queue1->push_back(l.backgrounds.second);

	door.setCoords(l.doorLocation.first, l.doorLocation.second);
	displayEntity(q.queue3, q.sizeQueue3, door, 0.65);
	cam1.setCoords(l.cameraLocation.first, l.cameraLocation.second);
	displayEntity(q.queue3, q.sizeQueue3, cam1, 0.4);
	cam2.setCoords(l.simulCameraLocation.first, l.simulCameraLocation.second);
	displayEntity(q.queue3, q.sizeQueue3, cam2, 0.4);

	for (LevelElement element : l.elements) {
		Entity* e = element.objptr;
		e->setCoords(element.elementX, element.elementY);
		e->setXPrime(element.velocities.first);
		e->setYPrime(element.velocities.second);

		switch (element.type)
		{
			case 'S':
			{
				Surface* s = dynamic_cast<Surface*>(e);
				displayEntity(q.queue5, q.sizeQueue5, q.animQueue, *s, element.size, element.animCode);
				break;
			}
			case 'B': 
			{
				Body* b = dynamic_cast<Body*>(e);
				displayEntity(q.queue4, q.sizeQueue4, q.hboxQueue, *b, element.size, element.hitbox);
				break;
			}
			case 'D':
			{
				displayEntity(q.queue2, q.sizeQueue2, *e, element.size);
				break;
			}				
			case 'E':
			{
				displayEntity(q.queue3, q.sizeQueue3, *e, element.size);
				break;
			}
		}
	}

	p.jump(0);
} // Sets up the objects in the levels to be rendered.

// Constants

const float PI = 3.14159265;
const float EAST = 0, NORTH = PI / 2, WEST = PI, SOUTH = 3*PI / 2;
const float NORTHEAST = PI / 4, NORTHWEST = 3*PI / 4, SOUTHWEST = 5*PI / 4, SOUTHEAST = 7*PI / 4;
const int WINDOW_WIDTH = 1400, WINDOW_HEIGHT = 750;
const pair<float,float> OFFSCREEN_COORDINATES = {-1000,-1000};
const pair<float,float> CENTER = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};

int main(int argc, char* args[])
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) > 0)
		std::cout << "SDL SYSTEM FAILURE. ERROR: " << SDL_GetError() << '\n';
	if (!(IMG_Init(IMG_INIT_PNG)))
		std::cout << "SDL IMAGE FAILURE. ERROR: " << SDL_GetError() << '\n';
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    	std::cout << "SDL AUDIO FAILURE. ERROR: " << Mix_GetError() << '\n';

    // Game handling variables

	SDL_Event event; // For basic keyboard input handling.
	bool running = true; // To indicate that the game is currently running.
	int tickRate = 100000; // tickRate is in ticks per second. A "tick" is the shortest unit of in-game time.
	int gameState = 2; // gameState dictates the player's degree of control based on what is being displayed - 0 indicates platforming/active gameplay, 1 indicates a cutscene during which the player cannot be controlled and may not be displayed, and 2 indicates the title screen.
	int currentLevel = 0; // level = 0 means not in a level.
	int targetTime[10]{};
	int timer = 0; // Starts running immediately and never stops.
	char titleLayer = 'T'; // T=title, P=play options, L=level select, C=controls, R=credits.
	char cutsceneCode = 'N'; // N=none, O=opening cutscene, A=camera activation, D=camera deactivation, 1=time dilation tutorial, 2=length contraction tutorial, 3=relativity of simultaneity tutorial, E=ending cutscene.
	bool nextLevel = false;
	bool playerDied = false;

	const Uint8* keystate = SDL_GetKeyboardState(nullptr); 
	bool leftPressed = keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A], rightPressed = keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D];
	int mouseX, mouseY;
	bool cutsceneContinue;
	int cutsceneTimer = 0;
	float trainFrameDelay = 0.01;

	float a = 10, b = 0;
	int c = 3, d = 3; // For animation.

	// Sprites and Entities

	RenderWindow window("Untitled Relativity Game", WINDOW_WIDTH, WINDOW_HEIGHT);

	SDL_Texture* player = window.loadTexture("res/gfx/miscellaneous/pixelpic2.png"); // Not to be confused with thePlayer.
	SDL_Texture* chalkboard = window.loadTexture("res/gfx/decoration/gamma.png");
	SDL_Texture* spaceBackground = window.loadTexture("res/gfx/backgrounds/Stars.png");
	SDL_Texture* windowBackground = window.loadTexture("res/gfx/backgrounds/backdrop1.png");
	SDL_Texture* indoorBackground = window.loadTexture("res/gfx/backgrounds/backdrop2.png");
	SDL_Texture* windowBackgroundSansFloor = window.loadTexture("res/gfx/backgrounds/backdrop1.5.png");
	SDL_Texture* nullSprite = window.loadTexture("res/gfx/miscellaneous/BLANK_ICON.png");
	SDL_Texture* earth = window.loadTexture("res/gfx/objects/Earth.png");
	SDL_Texture* basicPlatform = window.loadTexture("res/gfx/objects/platform2.png");
	SDL_Texture* shinyPlatform = window.loadTexture("res/gfx/objects/blueplatform.png");


	SDL_Texture* spriteArray[6] = {player, chalkboard, spaceBackground, nullSprite, windowBackground, indoorBackground}; // Packaging textures into arrays helps with organization.

	SDL_Texture* electrosphere[4] = {window.loadTexture("res/gfx/objects/electrosphere_off.png"), window.loadTexture("res/gfx/objects/electrosphere1.png"), window.loadTexture("res/gfx/objects/electrosphere2.png"), window.loadTexture("res/gfx/objects/electrosphere3.png")}; // An array can also used to package the different "costumes" (as the term is used in Scratch) of a single sprite.
	SDL_Texture* flamethrowerBase[4] = {window.loadTexture("res/gfx/objects/flamethrower_off.png"), window.loadTexture("res/gfx/objects/flamethrower_off_down.png"), window.loadTexture("res/gfx/objects/flamethrower_off_left.png"), window.loadTexture("res/gfx/objects/flamethrower_off_up.png")};
	SDL_Texture* flamethrowerFire[8] = {window.loadTexture("res/gfx/objects/flame1.png"), window.loadTexture("res/gfx/objects/flame2.png"), window.loadTexture("res/gfx/objects/flame1_down.png"), window.loadTexture("res/gfx/objects/flame2_down.png"), window.loadTexture("res/gfx/objects/flame1_left.png"), window.loadTexture("res/gfx/objects/flame2_left.png"), window.loadTexture("res/gfx/objects/flame1_up.png"), window.loadTexture("res/gfx/objects/flame2_up.png")};
	SDL_Texture* flamethrower[3] = {window.loadTexture("res/gfx/objects/flamethrower_off.png"), window.loadTexture("res/gfx/objects/flamethrower1.png"), window.loadTexture("res/gfx/objects/flamethrower2.png")}; // unused
	SDL_Texture* missileTextures[2] = {window.loadTexture("res/gfx/objects/missile.png"), window.loadTexture("res/gfx/objects/missile3.png")};
	SDL_Texture* explosion = window.loadTexture("res/gfx/objects/explosion (2)(2).png");

	SDL_Texture* door[3] = {window.loadTexture("res/gfx/objects/door_locked.png"), window.loadTexture("res/gfx/objects/door.png"), window.loadTexture("res/gfx/objects/door_open.png")};
	SDL_Texture* cameraLaptop[2] = {window.loadTexture("res/gfx/objects/camera1.png"), window.loadTexture("res/gfx/objects/camera2.png")};
	SDL_Texture* simulCameraLaptop[2] = {window.loadTexture("res/gfx/objects/camera3.png"), window.loadTexture("res/gfx/objects/camera4.png")};
	SDL_Texture* tutorialPoint[2] = {window.loadTexture("res/gfx/objects/tutorial2.png"), window.loadTexture("res/gfx/objects/tutorial2_large.png")};

	// Player Sprite

	SDL_Texture* playerWalk[10] = {window.loadTexture("res/gfx/player/i1.png"), window.loadTexture("res/gfx/player/i2.png"), window.loadTexture("res/gfx/player/i3.png"), window.loadTexture("res/gfx/player/i4.png"), window.loadTexture("res/gfx/player/i5.png"), window.loadTexture("res/gfx/player/i6.png"), window.loadTexture("res/gfx/player/i7.png"), window.loadTexture("res/gfx/player/i8.png"), window.loadTexture("res/gfx/player/i9.png"), window.loadTexture("res/gfx/player/i10.png")};
	int playerWidth[10] = {198,115,109,161,223,175,107,109,199,255};
	int playerHeight[10] = {336,343,352,340,346,336,343,352,340,346}; // The player's height and width will be dynamically updated according to the png dimensions of each frame, but that's probably fine.
	SDL_Texture* playerHurt = window.loadTexture("res/gfx/player/h1.png");
	SDL_Texture* playerSleep = window.loadTexture("res/gfx/player/s1.png");
	SDL_Texture* playerLook = window.loadTexture("res/gfx/player/l1.png");
	int playerSW = 352, playerSH = 106;
	int playerLW = 140, playerLH = 355;

	// Text and Button Sprites

	SDL_Texture* titleBlock = window.loadTexture("res/gfx/text/titleV1.png");
	SDL_Texture* creditsBlock = window.loadTexture("res/gfx/text/credits.png");
	SDL_Texture* controlsBlock = window.loadTexture("res/gfx/text/controls.png");

	SDL_Texture* playButton = window.loadTexture("res/gfx/buttons-info/playbutton.png");
	SDL_Texture* newGameButton = window.loadTexture("res/gfx/buttons-info/newgamebutton.png");
	SDL_Texture* levelSelectButton = window.loadTexture("res/gfx/buttons-info/levelselectbutton.png");
	SDL_Texture* controlsButton = window.loadTexture("res/gfx/buttons-info/controlsButton.png");
	SDL_Texture* creditsButton = window.loadTexture("res/gfx/buttons-info/creditsbutton.png");
	SDL_Texture* musicButton[2] = {window.loadTexture("res/gfx/buttons-info/music on.png"), window.loadTexture("res/gfx/buttons-info/music off.png")};
	SDL_Texture* soundButton[2] = {window.loadTexture("res/gfx/buttons-info/sound on.png"), window.loadTexture("res/gfx/buttons-info/sound off.png")};
	SDL_Texture* backButton = window.loadTexture("res/gfx/buttons-info/backarrow.png");

	SDL_Texture* emptyHealthBar = window.loadTexture("res/gfx/buttons-info/healthbar4.png");
	SDL_Texture* healthBar[3] = {window.loadTexture("res/gfx/buttons-info/healthbar1.png"), window.loadTexture("res/gfx/buttons-info/healthbar2.png"), window.loadTexture("res/gfx/buttons-info/healthbar3.png")};
	SDL_Texture* levelButton[12] = {window.loadTexture("res/gfx/buttons-info/level1.png"), window.loadTexture("res/gfx/buttons-info/level2.png"), window.loadTexture("res/gfx/buttons-info/level3.png"), window.loadTexture("res/gfx/buttons-info/level4.png"), window.loadTexture("res/gfx/buttons-info/level5.png"), window.loadTexture("res/gfx/buttons-info/level6.png"), window.loadTexture("res/gfx/buttons-info/level7.png"), window.loadTexture("res/gfx/buttons-info/level8.png"), window.loadTexture("res/gfx/buttons-info/level9.png"), window.loadTexture("res/gfx/buttons-info/level10.png"), window.loadTexture("res/gfx/buttons-info/level11.png"), window.loadTexture("res/gfx/buttons-info/level12.png")};

	// Cutscene Sprites

	SDL_Texture* cameraPlatform[4] = {window.loadTexture("res/gfx/objects/camera_station_1.png"), window.loadTexture("res/gfx/objects/camera_station_2.png"), window.loadTexture("res/gfx/objects/camera_station_3.png"), window.loadTexture("res/gfx/objects/camera_station_activated.png")};

	SDL_Texture* bgClouds = window.loadTexture("res/gfx/objects/clouds.png");
	SDL_Texture* bgBed = window.loadTexture("res/gfx/objects/bed.png");
	SDL_Texture* bgLever[2] = {window.loadTexture("res/gfx/objects/lever1.png"), window.loadTexture("res/gfx/objects/lever2.png")};
	SDL_Texture* bgPlatform = window.loadTexture("res/gfx/objects/elevated platform.png");
	SDL_Texture* bgWindow = window.loadTexture("res/gfx/objects/small window.png");
	SDL_Texture* bgPlanet = window.loadTexture("res/gfx/objects/Planet2.png");

	SDL_Texture* skyBG = window.loadTexture("res/gfx/backgrounds/Blue Sky 2 .png");
	SDL_Texture* cityBG = window.loadTexture("res/gfx/backgrounds/City.png");
	SDL_Texture* stationBG = window.loadTexture("res/gfx/backgrounds/Station Interior.png");
	SDL_Texture* galaxyBG = window.loadTexture("res/gfx/backgrounds/Galaxy.png");
	SDL_Texture* tutorialBG[5] = {window.loadTexture("res/gfx/backgrounds/tutorial1_1.png"), window.loadTexture("res/gfx/backgrounds/tutorial1_2.png"), window.loadTexture("res/gfx/backgrounds/tutorial1_3.png"), window.loadTexture("res/gfx/backgrounds/tutorial2.png"), window.loadTexture("res/gfx/backgrounds/tutorial3.png")};

	SDL_Texture* trainFrames[8] = {window.loadTexture("res/gfx/train/train1.png"), window.loadTexture("res/gfx/train/train2.png"), window.loadTexture("res/gfx/train/train3.png"), window.loadTexture("res/gfx/train/train4.png"), window.loadTexture("res/gfx/train/train5.png"), window.loadTexture("res/gfx/train/train6.png"), window.loadTexture("res/gfx/train/train7.png"), window.loadTexture("res/gfx/train/train8.png")};
	SDL_Texture* trainCar[3] = {window.loadTexture("res/gfx/train/middle .png"), window.loadTexture("res/gfx/train/middle with player.png"), window.loadTexture("res/gfx/train/back.png")};
	SDL_Texture* trainCarTop[2] = {window.loadTexture("res/gfx/train/car top.png"), window.loadTexture("res/gfx/train/car top with chain.png")};
	SDL_Texture* frontFacingTrain[2] = {window.loadTexture("res/gfx/train/front_train.png"), window.loadTexture("res/gfx/train/front_train_staircase.png")};

	SDL_Texture* tutorialText1[7] = {window.loadTexture("res/gfx/text/text11.png"), window.loadTexture("res/gfx/text/text12.png"), window.loadTexture("res/gfx/text/text13.png"), window.loadTexture("res/gfx/text/text14.png"), window.loadTexture("res/gfx/text/text15.png"), window.loadTexture("res/gfx/text/text16.png"), window.loadTexture("res/gfx/text/text17.png")};
	SDL_Texture* tutorialText2[3] = {window.loadTexture("res/gfx/text/text21.png"), window.loadTexture("res/gfx/text/text22.png"), window.loadTexture("res/gfx/text/text23.png")};
	SDL_Texture* tutorialText3[4] = {window.loadTexture("res/gfx/text/text31.png"), window.loadTexture("res/gfx/text/text32.png"), window.loadTexture("res/gfx/text/text33.png"), window.loadTexture("res/gfx/text/text34.png")};

	// Entities

	const Entity nullEntity(0, 0, 0, 0, window.loadTexture("res/gfx/miscellaneous/BLANK_ICON.png")); // Is invisible.
	const Entity blackCover(0, 0, 0, 0, window.loadTexture("res/gfx/miscellaneous/Black.png")); // Is entirely black.
	const Entity whiteCover(0, 0, 0, 0, window.loadTexture("res/gfx/miscellaneous/White.png")); // Is entirely white. 
	const Entity implicitFloor(0, WINDOW_HEIGHT-50, WINDOW_WIDTH, 50, window.loadTexture("res/gfx/miscellaneous/BLANK_ICON.png"));
	const Entity implicitWallL(0, -1*WINDOW_HEIGHT, 5, 2*WINDOW_HEIGHT, window.loadTexture("res/gfx/miscellaneous/BLANK_ICON.png"));
	const Entity implicitWallR(WINDOW_WIDTH-5, -1*WINDOW_HEIGHT, 500, 2*WINDOW_HEIGHT, window.loadTexture("res/gfx/miscellaneous/BLANK_ICON.png"));
	const Entity implicitCeiling(0, 0, WINDOW_WIDTH, 5, window.loadTexture("res/gfx/miscellaneous/BLANK_ICON.png")); // These are invisible surfaces that prevent the player from falling through the floor or walls.

	Entity gammaBoard(250, 80, 711, 368, chalkboard);
	Entity gammaBoard2(500, 400, 711, 368, chalkboard);
	Entity gammaBoard3(800, 600, 711, 368, chalkboard);
	Entity redditIcon(100, 400, 200, 300, player);

	Entity playerIcon(100, 400, playerWidth[c], playerHeight[c], playerWalk[c]);
	Entity electroSphere(958, 270, 304, 304, electrosphere[1]);
	Entity flameThrower(100, 450, 366, 100, flamethrower[1]);
	Entity flameBaseR(0, 0, 100, 100, flamethrowerBase[0]);
	Entity flameBaseD(0, 0, 100, 100, flamethrowerBase[1]);
	Entity flameBaseL(0, 0, 100, 100, flamethrowerBase[2]);
	Entity flameBaseU(0, 0, 100, 100, flamethrowerBase[3]);
	Entity flameR(0, 0, 270, 99, flamethrowerFire[0]);
	Entity flameD(0, 0, 99, 270, flamethrowerFire[2]);
	Entity flameL(0, 0, 270, 99, flamethrowerFire[4]);
	Entity flameU(0, 0, 99, 270, flamethrowerFire[6]);
	Entity exitDoor(1185, 310, 134, 233, door[1]);
	Entity cameraActivator(100, 100, 280, 161, cameraLaptop[0]);
	Entity simulCameraActivator(100, 100, 280, 161, simulCameraLaptop[0]);
	Entity doorKey(0, 0, 160, 101, window.loadTexture("res/gfx/objects/key.png"));
	Entity healthPowerup(0, 0, 220, 167, window.loadTexture("res/gfx/objects/health_refill.png"));
	Entity platform(600, 460, 341, 48, basicPlatform);
	Entity bluePlatform(600, 460, 341, 48, shinyPlatform);
	Entity shortPlatform(0, 0, 207, 48, window.loadTexture("res/gfx/objects/platform1.png"));
	Entity longPlatform(0, 0, 469, 48, window.loadTexture("res/gfx/objects/platform3.png"));
	Entity longLongPlatform(0, 0, 808, 49, window.loadTexture("res/gfx/objects/beeg.png"));
	Entity redPlatform(100, 600, 341, 48, window.loadTexture("res/gfx/objects/redplatform.png"));
	Entity crate(0, 340, 310, 310, window.loadTexture("res/gfx/objects/crate2.png"));
	Entity crate2(0, 570, 310, 310, window.loadTexture("res/gfx/objects/crate2.png"));
	Entity crate3(230, 570, 310, 310, window.loadTexture("res/gfx/objects/crate2.png"));
	Entity wideCrate(200, 200, 506, 310, window.loadTexture("res/gfx/objects/crate1.png"));
	Entity blockPlatform(0, 0, 419, 193, window.loadTexture("res/gfx/objects/solidplatform.png"));
	Entity blockPlatformReversed(0, 0, 419, 193, window.loadTexture("res/gfx/objects/darkceil.png"));
	Entity blockPlatformReversedLight(0, 0, 419, 193, window.loadTexture("res/gfx/objects/upsidedownsolidplatform.png"));
	Entity factoryBlock(0, 0, 310, 310, window.loadTexture("res/gfx/objects/factoryblock1.png"));
	Entity factoryBlockWide(0, 0, 553, 355, window.loadTexture("res/gfx/objects/factoryblock2.png"));
	Entity factoryBlockTall(0, 0, 279, 605, window.loadTexture("res/gfx/objects/factoryblock3.png"));
	Entity factoryBarrier(900+7, -25, 40, 602, window.loadTexture("res/gfx/objects/thin wall.png"));
	Entity factoryBarrier2(1200-7, -25, 40, 602, window.loadTexture("res/gfx/objects/thin wall.png"));
	Entity electroBeam(200, 200, 570, 54, window.loadTexture("res/gfx/objects/electrobeam.png"));
	Entity shortElectroBeam(200, 200, 320, 54, window.loadTexture("res/gfx/objects/electrobeamshort.png"));
	Entity longElectroBeam(200, 200, 960, 54, window.loadTexture("res/gfx/objects/electrobeamlong.png"));
	Entity missileLauncher(200, 200, 100, 100, window.loadTexture("res/gfx/objects/missile launcher.png"));
	Entity missileShot(200, 200, 123, 39, missileTextures[0]);
	Entity kaboom(0, 0, 261, 259, explosion);
	Entity lightning(0, 0, 86, 334, window.loadTexture("res/gfx/objects/lightning.png"));
	Entity shortSupportBeam(0, 0, 34, 233, window.loadTexture("res/gfx/objects/short_beam.png"));
	Entity lsupportBeam(0, 0, 34, 415, window.loadTexture("res/gfx/objects/beam.png"));
	Entity rsupportBeam = lsupportBeam;
	Entity longSupportBeam(0, 0, 34, 533, window.loadTexture("res/gfx/objects/long_beam.png"));
	Entity verticalSupportBeam(0, 0, 34, 415, window.loadTexture("res/gfx/objects/verticalbeam.png"));
	Entity supportString(0, 0, 34, 533, window.loadTexture("res/gfx/objects/string.png"));
	Entity lsupportString = supportString;
	Entity rsupportString = supportString;
	lsupportBeam.setTilt(45);
	rsupportBeam.setTilt(-45);
	lsupportString.setTilt(20);
	rsupportString.setTilt(-20);

	Entity earthLarge(200, 400, 220, 220, earth);
	Entity tutorialHolo(0, 0, 223, 215, tutorialPoint[0]);
	Entity laptop(1000, 640, 280, 161, window.loadTexture("res/gfx/decoration/laptop.png"));
	Entity table(528, 540, 496, 256, window.loadTexture("res/gfx/decoration/table.png"));
	Entity tabletop(0, 0, 496, 133, window.loadTexture("res/gfx/decoration/tabletop.png"));
	Entity greenPotion(1150, 640, 54, 77, window.loadTexture("res/gfx/decoration/Potion-a.png"));
	Entity bluePotion(1210, 640, 53, 107, window.loadTexture("res/gfx/decoration/Potion-b.png"));
	Entity purplePotion(1270, 640, 54, 161, window.loadTexture("res/gfx/decoration/Potion-c.png"));
	Entity maxwellBoard(0, 0, 512, 345, window.loadTexture("res/gfx/decoration/maxwellBoard.png"));
	Entity lorentzBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/lorentzBoard.png"));
	Entity transformBoard(0, 0, 416, 257, window.loadTexture("res/gfx/decoration/transformBoard.png"));
	Entity logisticsBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/logisticsBoard.png"));
	Entity sourceCodeBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/sourceCodeBoard.png"));
	Entity einsteinBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/einsteinBoard.png"));
	Entity generalBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/generalBoard.png"));
	Entity invariantBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/invariantBoard.png"));
	Entity simulBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/simulBoard.png"));
	Entity brakeBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/brakeBoard.png"));
	Entity phiBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/phiBoard.png"));
	Entity keyBoard(0, 0, 416, 284, window.loadTexture("res/gfx/decoration/keyHint.png"));
	Entity cutsceneBoard(542, 56, 416, 284, window.loadTexture("res/gfx/decoration/introBoard.png"));
	Entity shade(0, 0, 960, 275, window.loadTexture("res/gfx/decoration/shade.png"));
	Entity sign1(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign1.png"));
	Entity sign2(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign2.png"));
	Entity sign3(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign3.png"));
	Entity sign4(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign4.png"));
	Entity sign5(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign5.png"));
	Entity sign6(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign6.png"));
	Entity sign7(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign7.png"));
	Entity sign8(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign8.png"));
	Entity sign9(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign9.png"));
	Entity sign10(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign10.png"));
	Entity sign11(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign11.png"));
	Entity sign12(0, 0, 186, 158, window.loadTexture("res/gfx/decoration/sign12.png"));
	setColour(maxwellBoard, 120, 0, 120);
	setTransparency(tutorialHolo, 128);

	Entity title(250, 60, 918, 250, titleBlock);
	Entity creditList(255, 30, 859, 677, creditsBlock);
	Entity controlsList(255, 30, 859, 677, controlsBlock);

	Entity play(210, 460, 240, 104, playButton);
	Entity controls(580, 460, 240, 104, controlsButton);
	Entity credits(950, 460, 240, 104, creditsButton);
	Entity newGame(350, 320, 240, 104, newGameButton);
	Entity levelSelect(810, 320, 240, 104, levelSelectButton);
	Entity musicToggle(1180, 640, 90, 90, musicButton[0]);
	Entity soundToggle(1290, 640, 90, 90, soundButton[0]);
	Entity back(35, 35, 47, 39, backButton);
	Entity health(50, 35, 551, 282, healthBar[2]);

	Entity level1(100, 150, 90, 90, levelButton[0]);
	Entity level2(100+215, 150, 90, 90, levelButton[1]);
	Entity level3(100+2*215, 150, 90, 90, levelButton[2]);
	Entity level4(100+3*215, 150, 90, 90, levelButton[3]);
	Entity level5(100+4*215, 150, 90, 90, levelButton[4]);
	Entity level6(100+5*215, 150, 90, 90, levelButton[5]);
	Entity level7(100, 450, 90, 90, levelButton[6]);
	Entity level8(100+215, 450, 90, 90, levelButton[7]);
	Entity level9(100+2*215, 450, 90, 90, levelButton[8]);
	Entity level10(100+3*215, 450, 90, 90, levelButton[9]);
	Entity level11(100+4*215, 450, 90, 90, levelButton[10]);
	Entity level12(100+5*215, 450, 90, 90, levelButton[11]);
	Entity levels[] = {level1, level2, level3, level4, level5, level6, level7, level8, level9, level10, level11, level12};

	Entity backgrounda(0, 0, 100, 100, spriteArray[2]);
	Entity backgroundb(0, 0, 100, 100, spriteArray[4]);
	Entity backgroundc(0, 0, 100, 100, spriteArray[5]);
	Entity backgroundd(0, 0, 100, 100, window.loadTexture("res/gfx/backgrounds/backdrop3.png"));
	Entity backgrounde(0, 0, 100, 100, windowBackgroundSansFloor);
	Entity titleBackground(0, 0, 100, 100, window.loadTexture("res/gfx/backgrounds/art1.png"));
	Entity lens(0, 0, 100, 100, window.loadTexture("res/gfx/backgrounds/lens1.png"));
	Entity lensrec(0, 0, 100, 100, window.loadTexture("res/gfx/backgrounds/lens2.png"));
	setTransparency(lens, 40);
	setTransparency(lensrec, 40);
	lensrec.toggleVisible();

	Entity cameraStation(150, 250, 761, 210, cameraPlatform[0]);
	Entity lever(750, 557, 96, 92, bgLever[0]);
	Entity bed(50, 500, 287, 209, bgBed);
	Entity clouds(200, 800, 751, 425, bgClouds);
	Entity station(1150, 440, 227, 269, window.loadTexture("res/gfx/objects/station.png"));
	Entity cutscenePlayer(-100, 620, playerWidth[d], playerHeight[d], playerWalk[d]);
	Entity date(CENTER.first-350, CENTER.second, 633, 80, window.loadTexture("res/gfx/text/date.png"));
	Entity tbc(CENTER.first-350, CENTER.second, 635, 60, window.loadTexture("res/gfx/text/tbc.png"));
	Entity elevatedPlatform(540, 590, 830, 422, bgPlatform);
	Entity planet(0, 0, 207, 100, bgPlanet);
	Entity miniWindow(561, 75, 142, 84, bgWindow);
	Entity cutsceneBG(0, 0, 100, 100, skyBG);
	Entity cutsceneBG2(0, 0, 100, 100, cityBG);
	Entity cutsceneFrontTrain(1107, 280, 159, 309, frontFacingTrain[0]);
	Entity cutsceneSideTrain(CENTER.first-180, CENTER.second-147, 481, 316, trainFrames[0]);
	Entity cutsceneRearCar(CENTER.first-275*3 + 38, CENTER.second, 266, 162, trainCar[2]);
	Entity cutsceneMiddleCarn2(CENTER.first-275*4, CENTER.second, 302, 162, trainCar[0]);
	Entity cutsceneMiddleCarn1(CENTER.first-275*3, CENTER.second, 302, 162, trainCar[0]);
	Entity cutsceneMiddleCar0(CENTER.first-275*2, CENTER.second, 302, 162, trainCar[0]);
	Entity cutsceneMiddleCar1(CENTER.first-275, CENTER.second, 302, 162, trainCar[0]);
	Entity cutsceneMiddleCar2(CENTER.first+275, CENTER.second, 302, 162, trainCar[0]);
	Entity cutsceneMiddleCar3(CENTER.first+275*2, CENTER.second, 302, 162, trainCar[0]);
	Entity cutscenePlayerCar(CENTER.first, CENTER.second, 302, 162, trainCar[1]);
	Entity cutsceneCarTop(0, 0, 334, 267, trainCarTop[0]);
	Entity backgroundt(0, 0, 100, 100, tutorialBG[0]);
	Entity frontTracks(1124, 676, 556, 112, window.loadTexture("res/gfx/train/track_front_view.png"));
	Entity sideTracks0(-684, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks1(0, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks2(684, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks3(684*2, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks4(684*3, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks5(684*4, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks6(684*5, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks7(684*6, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks8(684*7, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks9(684*8, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks10(684*9, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks11(684*10, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks12(684*11, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks13(684*12, 530, 904, 81, window.loadTexture("res/gfx/train/track_side_view.png"));
	Entity sideTracks[14] = {sideTracks0, sideTracks1, sideTracks2, sideTracks3, sideTracks4, sideTracks5, sideTracks6, sideTracks7, sideTracks8, sideTracks9, sideTracks10, sideTracks11, sideTracks12, sideTracks13};
	Entity topTracks(0, 570, 960, 154, window.loadTexture("res/gfx/train/track1.png"));
	Entity topTracksTilted(0, 570, 960, 604, window.loadTexture("res/gfx/train/track2.png"));
	Entity staircase(980, 540, 95, 195, window.loadTexture("res/gfx/train/staircase.png"));
	Entity continuePrompt(1130, 674, 73, 46, window.loadTexture("res/gfx/buttons-info/continue prompt.png"));
	continuePrompt.toggleVisible();

	Entity tutorialText11(225, 540, 953, 81, tutorialText1[0]);
	Entity tutorialText12(225+250, 540, 555, 81, tutorialText1[1]);
	Entity tutorialText13(225+250, 540+110, 955, 149, tutorialText1[2]);
	Entity tutorialText14(225, 540+120, 927, 81, tutorialText1[3]);
	Entity tutorialText15(225-150, 540, 846, 75, tutorialText1[4]);
	Entity tutorialText16(225-150, 540, 879, 127, tutorialText1[5]);
	Entity tutorialText17(225-150, 540, 935, 81, tutorialText1[6]);

	Entity tutorialText21(225, 540, 942, 167, tutorialText2[0]);
	Entity tutorialText22(225, 540, 882, 121, tutorialText2[1]);
	Entity tutorialText23(225, 540, 904, 81, tutorialText2[2]);

	Entity tutorialText31(255, 615, 834, 81, tutorialText3[0]);
	Entity tutorialText32(255, 615, 902, 81, tutorialText3[1]);
	Entity tutorialText33(255, 615, 908, 127, tutorialText3[2]);
	Entity tutorialText34(255, 645, 910, 34, tutorialText3[3]);

	Entity tutorialTextArray1[7] = {tutorialText11, tutorialText12, tutorialText13, tutorialText14, tutorialText15, tutorialText16, tutorialText17};
	Entity tutorialTextArray2[3] = {tutorialText21, tutorialText22, tutorialText23};
	Entity tutorialTextArray3[4] = {tutorialText31, tutorialText32, tutorialText33, tutorialText34};


	Body movingBody(gammaBoard, 8, 0, false, true, 0.5);
	movingBody.setCoords(190, 350);
	Body anotherMovingBody(gammaBoard2, 15, 0, false, true, 0.2);
	anotherMovingBody.changeY(-250);

	float playerSize = 0.4;
	Body thePlayer(playerIcon, 0, 0, false, false, playerSize); // The player.
	int HP = 3, maxSpeed = 30;
	bool grounded = false, facing = true, iFrame = false; // You can only jump if grounded. facing=1 => right, facing=0 => left.
	bool touchingPlatform = false, exitDoorOpen = false;
	float platformBorderL = -1000, platformBorderR = 3000, platformBorderY = -1000; 
	int landedIndex = -1;
	char landedType = 'n'; // For none, while 'b' means body and 's' means surface.
	int j = 0;

	thePlayer.setCoords(600, 100);
	thePlayer.jump(0);


	Surface platform1(platform, true, true, true, true, 0);
	Surface platform2(platform, false, false, false, true, 0); // Has the same collision as a semisolid platform/lift from a 2D mario game.
	Surface platform3(redPlatform, true, true, true, true, 1); // Deals damage on contact.
	platform3.setBouncy();
	platform3.addVelVector(EAST, 12);
	platform2.changeX(400);
	platform1.changeX(-100);
	platform1.changeY(-250);

	Surface solidShort(shortPlatform, true, true, true, true, 0);
	Surface semisolidShort(shortPlatform, false, false, false, true, 0);
	Surface solidPlatform(platform, true, true, true, true, 0);
	Surface semisolidPlatform(platform, false, false, false, true, 0);
	Surface solidLong(longPlatform, true, true, true, true, 0);
	Surface semisolidLong(longPlatform, false, false, false, true, 0);
	Surface solidLongLong(longLongPlatform, true, true, true, true, 0);
	Surface blueSolidPlatform(bluePlatform, true, true, true, true, 0);
	Surface largeCrate(wideCrate, true, true, true, true, 0);
	Surface solidBlock(blockPlatform, true, true, true, true, 0);
	Surface solidBlockR(blockPlatformReversed, true, true, true, true, 0);
	Surface solidBlockRL(blockPlatformReversedLight, true, true, true, true, 0);
	Surface metalCrate(factoryBlock, true, true, true, true, 0);
	Surface metalCrateWide(factoryBlockWide, true, true, true, true, 0);
	Surface metalCrateTall(factoryBlockTall, true, true, true, true, 0);
	Surface thinWall(factoryBarrier, true, true, true, true, 0);
	solidShort.makePlatform();
	semisolidShort.makePlatform();
	solidPlatform.makePlatform();
	semisolidPlatform.makePlatform();
	solidLong.makePlatform();
	semisolidLong.makePlatform();
	solidLongLong.makePlatform();
	blueSolidPlatform.makePlatform();

	solidShort.setBouncy();
	solidPlatform.setBouncy();
	semisolidPlatform.setBouncy();
	semisolidShort.setBouncy();

	Surface exitKey(doorKey, false, false, false, false, 0);
	Surface healthRefill(healthPowerup, false, false, false, 0);
	Surface flameThrowerAnimated(flameThrower, false, false, false, false, 1);
	Surface electroSphereAnimated(electroSphere, false, false, false, false, 1);
	Surface flameContainerR(flameBaseR, true, true, true, true, 0);
	Surface flameContainerD(flameBaseD, true, true, true, true, 0);
	Surface flameContainerL(flameBaseL, true, true, true, true, 0);
	Surface flameContainerU(flameBaseU, true, true, true, true, 0);
	Surface flameBurstR(flameR, false, false, false, false, 1);
	Surface flameBurstD(flameD, false, false, false, false, 1);
	Surface flameBurstL(flameL, false, false, false, false, 1);
	Surface flameBurstU(flameU, false, false, false, false, 1);
	Surface elecBeamShort(shortElectroBeam, false, false, false, false, 1);
	Surface elecBeam(electroBeam, false, false, false, false, 1);
	Surface elecBeamLong(longElectroBeam, false, false, false, false, 1);
	Surface missileCannon(missileLauncher, true, true, true, true, 0);
	Surface missile(missileShot, false, false, false, false, 1);
	Surface lightningBeam(lightning, false, false, false, false, 1);
	Surface leftMissile = missile;
	Surface upMissile = missile;
	Surface dmgPlatform = platform3;
	dmgPlatform.makePlatform();
	dmgPlatform.setBouncy();
	leftMissile.setTilt(180);
	upMissile.setTilt(90);

	const Surface floorInvis(implicitFloor, true, true, true, true, 0);
	const Surface wallL(implicitWallL, true, true, true, true, 0);
	const Surface wallR(implicitWallR, true, true, true, true, 0);
	const Surface ceilingInvis(implicitCeiling, true, true, true, true, 0);

	// Entity queues to be rendered, in which order matters - the objects at the front of the vector will be on the back layer, and those at the end of the vector will be on the front layer. A vector in C++ is basically the same as a list in Python.

	vector<Entity> backgroundRenderQueue = {backgrounda, backgroundb};
	backgroundRenderQueue.reserve(10); 

	vector<Entity> backgroundObjRenderQueue = {laptop}; 
	vector<float> backgroundObjRenderSize = {0.4};
	backgroundObjRenderQueue.reserve(50);
	backgroundObjRenderSize.reserve(50);

	vector<Entity> objectRenderQueue = {nullEntity, redditIcon, electroSphere}; 
	vector<float> objectRenderSize = {4.0, 0.5, 0.25};
	objectRenderQueue.reserve(100);
	objectRenderSize.reserve(100);

	vector<Body> bodyRenderQueue = {movingBody};
	vector<float> bodyRenderSize = {0.5};
	vector<bool> bodyHasHitbox = {true}; 
	bodyRenderQueue.reserve(50);
	bodyRenderSize.reserve(50);
	bodyHasHitbox.reserve(50);

	vector<Surface> surfaceRenderQueue = {}; 
	vector<float> surfaceRenderSize = {};
	vector<char> surfaceAnimationCode = {};
	surfaceRenderQueue.reserve(50);
	surfaceRenderSize.reserve(50);
	surfaceAnimationCode.reserve(50); 

	QueueSet renderQueues = {
		&backgroundRenderQueue, 
		&backgroundObjRenderQueue,
		&objectRenderQueue, 
		&bodyRenderQueue,
		&surfaceRenderQueue, 
		&backgroundObjRenderSize, 
		&objectRenderSize, 
		&bodyRenderSize, 
		&surfaceRenderSize, 
		&bodyHasHitbox, 
		&surfaceAnimationCode
	};

	displayEntity(&objectRenderQueue, &objectRenderSize, exitDoor, 0.65);
	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, platform1);
	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, platform2);
	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, platform3);

	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, floorInvis);
	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallL);
	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallR);
	displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, floorInvis); // The entities rendered here initially do not show up in game, but were used for testing.

	// Audio

	Mix_Music* guardian = Mix_LoadMUS("res/sfx/music/Guardian.wav");
	Mix_Music* jugglingFire = Mix_LoadMUS("res/sfx/music/Juggling Fire.wav");
	Mix_Music* rush = Mix_LoadMUS("res/sfx/music/Rush.wav");
	Mix_Music* moonstruckMash = Mix_LoadMUS("res/sfx/music/Moonstruck Mash.wav");
	Mix_Music* nostalgia = Mix_LoadMUS("res/sfx/music/Nostalgia.wav");
	Mix_Music* unnamed2 = Mix_LoadMUS("res/sfx/music/Unnamed 2.wav");
	Mix_Music* unnamed3 = Mix_LoadMUS("res/sfx/music/Unnamed 3.wav");

	Mix_Chunk* trainWhistle = Mix_LoadWAV("res/sfx/sounds/Train Whistle.wav");
	Mix_Chunk* trainNoise = Mix_LoadWAV("res/sfx/sounds/Sewing Machine.wav");
	Mix_Chunk* awe = Mix_LoadWAV("res/sfx/sounds/Boom Cloud.wav");

	map<string, Mix_Music*> soundtrack = {{"Title Screen", jugglingFire}, {"Gameplay", rush}, {"Opening Cutscene", unnamed2}}; // The soundtrack, from the Crescent Eclipse OST.
	map<string, Mix_Chunk*> soundEffects = {{"Train Whistle", trainWhistle}, {"Train Noise", trainNoise}, {"Realization", awe}}; // Various sound effects.

	soundtrack["Hint"] = unnamed3;
	soundtrack["Unused Track"] = guardian;
	soundtrack["Unused Track 2"] = moonstruckMash;
	soundtrack["Ending Cutscene"] = nostalgia;

	soundEffects["Ding"] = Mix_LoadWAV("res/sfx/sounds/Coin.wav");
	soundEffects["Door Open"] = Mix_LoadWAV("res/sfx/sounds/door open.wav");
	soundEffects["Game Over"] = Mix_LoadWAV("res/sfx/sounds/Oops.wav");
	soundEffects["Crash"] = Mix_LoadWAV("res/sfx/sounds/Crunch.wav");
	soundEffects["Whoosh"] = Mix_LoadWAV("res/sfx/sounds/Low Whoosh.wav");
	soundEffects["Materialize"] = Mix_LoadWAV("res/sfx/sounds/materialize.wav");
	soundEffects["Missile Shot"] = Mix_LoadWAV("res/sfx/sounds/Missile Launch.wav");
	soundEffects["Crash"] = Mix_LoadWAV("res/sfx/sounds/Crunch.wav");
	soundEffects["Engine Shutdown"] = Mix_LoadWAV("res/sfx/sounds/Shutdown.wav");
	soundEffects["Space Ambience"] = Mix_LoadWAV("res/sfx/sounds/Space Noise.wav");
	soundEffects["Inquisition"] = Mix_LoadWAV("res/sfx/sounds/Suspense.wav");
	soundEffects["Star Shine"] = Mix_LoadWAV("res/sfx/sounds/Teleport3.wav");
	soundEffects["Text Reading"] = Mix_LoadWAV("res/sfx/sounds/Voice SFX 3.wav");
	soundEffects["Whir"] = Mix_LoadWAV("res/sfx/sounds/Whir.wav");
	soundEffects["Hurt"] = Mix_LoadWAV("res/sfx/sounds/Wobble.wav");
	soundEffects["Jump"] = Mix_LoadWAV("res/sfx/sounds/Jump.wav");
	soundEffects["Accelerate"] = Mix_LoadWAV("res/sfx/sounds/Accelerate.wav");
	soundEffects["Train Accelerate"] = Mix_LoadWAV("res/sfx/sounds/trainAccel.wav");
	soundEffects["Station Bell"] = Mix_LoadWAV("res/sfx/sounds/mixkit-classic-melodic-clock-strike-1058.wav");
	soundEffects["Indoor Ambience"] = Mix_LoadWAV("res/sfx/sounds/mixkit-industrial-hum-loop-2139.wav");
	soundEffects["Level Complete"] = Mix_LoadWAV("res/sfx/sounds/mixkit-retro-game-notification-212.wav");
	soundEffects["Activate"] = Mix_LoadWAV("res/sfx/sounds/Connect.wav");
	soundEffects["Deactivate"] = Mix_LoadWAV("res/sfx/sounds/Disconnect.wav");
	soundEffects["Lightning"] = Mix_LoadWAV("res/sfx/sounds/mixkit-explosion-hit-1704.wav");
	soundEffects["Flame Burst"] = Mix_LoadWAV("res/sfx/sounds/WU_SE_OBJ_FIRE_CANNON_BLAZE.wav");
	soundEffects["Zap"] = Mix_LoadWAV("res/sfx/sounds/mixkit-small-metallic-sci-fi-drop-888.wav");
	soundEffects["Heal"] = Mix_LoadWAV("res/sfx/sounds/Magic Spell.wav");
	soundEffects["Ticking"] = Mix_LoadWAV("res/sfx/sounds/ticking.wav");
	soundEffects["Restart"] = Mix_LoadWAV("res/sfx/sounds/restart.wav");
	soundEffects["Quit to Title"] = Mix_LoadWAV("res/sfx/sounds/quit to title.wav");
	soundEffects["Tutorial"] = Mix_LoadWAV("res/sfx/sounds/mixkit-interface-hint-notification-911.wav");

	int musicVolume = 16, soundVolume = 32; // These seem like good default volumes.

	Mix_Volume(-1,soundVolume); 
	Mix_VolumeMusic(musicVolume);

	// Relativity

	FrameOfReference train = {true, 0.7*SPEED_OF_LIGHT};
	FrameOfReference camera = {false, 0.1*SPEED_OF_LIGHT};
	FrameOfReference simulCamera = {false, -0.1*SPEED_OF_LIGHT};

	bool relativityOn = false;
	float gamma = lorentzFactor(train, camera); // Equal to 1.25 at the start of the game.
	float playerLengthContraction = 1.0;// The factors by which spatial dimensions are contracted for the player. Other entities, if they move, are simply have their length divided by gamma.
	Uint8 redshiftAmount = 0, blueshiftAmount = 0; // The magnitudes of the relativistic doppler effect.

	// Levels 

	LevelElement level1_lowPlatform = {&solidLongLong, 'S', '\0', {0,500}, {0,0}, 1.0};
	LevelElement level1_highPlatform = {&solidLongLong, 'S', '\0', {600,300}, {0,0}, 1.0};
	LevelElement level1_decoBoard = {&maxwellBoard, 'D', '\0', {583,40}, {0,0}, 0.55};
	LevelElement level1_table = {&table, 'D', '\0', {1145,595}, {0,0}, 0.40};
	LevelElement level1_potion1 = {&greenPotion, 'D', '\0', {1180,560}, {0,0}, 0.5};
	LevelElement level1_potion2 = {&bluePotion, 'D', '\0', {1230,545}, {0,0}, 0.5};
	LevelElement level1_potion3 = {&purplePotion, 'D', '\0', {1280,518}, {0,0}, 0.5};
	LevelElement level1_beam1 = {&lsupportBeam, 'E', '\0', {50,500}, {0,0}, 0.5};
	LevelElement level1_beam2 = {&rsupportBeam, 'E', '\0', {1350,300}, {0,0}, 0.5};
	LevelElement level1_sign = {&sign1, 'D', '\0', {380,596}, {0,0}, 0.65};

	Level level_1 = {
		0.4, // playerSize
		true, false, true, true, // implicit borders
		false, // door locked
		{backgroundc, backgroundc}, // backgrounds
		{100, 560}, // player coordinates
		{1255, 150}, // door coordinates
		OFFSCREEN_COORDINATES, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level1_lowPlatform, level1_highPlatform, level1_decoBoard, level1_table, level1_potion1, level1_potion2, level1_potion3, level1_beam1, level1_beam2, level1_sign} // elements
	};


	LevelElement level2_bottomCrate = {&largeCrate, 'S', '\0', {440,510}, {0,0}, 0.6};
	LevelElement level2_bottomCrate2 = {&largeCrate, 'S', '\0', {740,510}, {0,0}, 0.6};
	LevelElement level2_topCrate = {&largeCrate, 'S', '\0', {580,330}, {0,0}, 0.6};
	LevelElement level2_platform = {&solidShort, 'S', '\0', {200,330}, {-10,0}, 0.9};
	LevelElement level2_decoBoard = {&lorentzBoard, 'D', '\0', {620,20}, {0,0}, 0.55};
	LevelElement level2_key = {&exitKey, 'S', 'K', {50,60}, {0,0}, 0.55};
	LevelElement level2_sign = {&sign2, 'D', '\0', {300,595}, {0,0}, 0.65};

	Level level_2 = {
		0.38, // playerSize
		true, true, true, true, // implicit borders
		true, // door locked
		{backgrounda, backgroundb}, // backgrounds 
		{60, 560}, // player coordinates
		{1255, 544}, // door coordinates
		OFFSCREEN_COORDINATES, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level2_bottomCrate, level2_bottomCrate2, level2_topCrate, level2_platform, level2_decoBoard, level2_key, level2_sign} // elements
	};


	LevelElement level3_leftPlatform = {&solidShort, 'S', '\0', {-35,200}, {0,0}, 1.0};
	LevelElement level3_middlePlatform = {&solidLong, 'S', '\0', {400,200}, {0,0}, 1.0};
	LevelElement level3_rightPlatform = {&solidShort, 'S', '\0', {1210,200}, {0,0}, 1.0};
	LevelElement level3_lbeam1 = {&verticalSupportBeam, 'E', '\0', {35,240}, {0,0}, 1.3};
	LevelElement level3_lbeam2 = {&verticalSupportBeam, 'E', '\0', {440,240}, {0,0}, 1.3};
	LevelElement level3_lbeam3 = {&verticalSupportBeam, 'E', '\0', {750,240}, {0,0}, 1.3};
	LevelElement level3_lbeam4 = {&verticalSupportBeam, 'E', '\0', {1290,240}, {0,0}, 1.3};
	LevelElement level3_string1 = {&supportString, 'E', '\0', {530,240}, {0,0}, 0.22};
	LevelElement level3_string2 = {&supportString, 'E', '\0', {690,240}, {0,0}, 0.22};
	LevelElement level3_decoBoard = {&transformBoard, 'D', '\0', {500,357}, {0,0}, 0.56};
	LevelElement level3_shade = {&shade, 'D', '\0', {0,350}, {0,0}, 2.0};
	LevelElement level3_sign = {&sign3, 'D', '\0', {51,118}, {0,0}, 0.65};

	Level level_3 = {
		0.4, // playerSize
		false, false, true, true, // implicit borders
		false, // door locked
		{backgroundd, backgroundd}, // backgrounds 
		{60, 50}, // player coordinates
		{1290, 50}, // door coordinates
		OFFSCREEN_COORDINATES, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level3_leftPlatform, level3_middlePlatform, level3_rightPlatform, level3_lbeam1, level3_lbeam2, level3_lbeam3, level3_lbeam4, level3_string1, level3_string2, level3_decoBoard, level3_shade, level3_sign} // elements
	};


	LevelElement level4_key = {&exitKey, 'S', 'K', {1250,550}, {0,0}, 0.55};
	LevelElement level4_ceiling = {&solidBlockR, 'S', '\0', {-20,-590}, {0,0}, 3.5};
	LevelElement level4_platform = {&semisolidLong, 'S', '\0', {-90,330}, {0,0}, 1.0};
	LevelElement level4_stair1 = {&semisolidShort, 'S', '\0', {260,380}, {0,0}, 1.0};
	LevelElement level4_stair2 = {&semisolidShort, 'S', '\0', {350,430}, {0,0}, 1.0};
	LevelElement level4_stair3 = {&semisolidShort, 'S', '\0', {440,480}, {0,0}, 1.0};
	LevelElement level4_stair4 = {&semisolidShort, 'S', '\0', {530,530}, {0,0}, 1.0};
	LevelElement level4_stair5 = {&semisolidShort, 'S', '\0', {620,580}, {0,0}, 1.0};
	LevelElement level4_stair6 = {&semisolidShort, 'S', '\0', {710,630}, {0,0}, 1.0};
	LevelElement level4_ceilingblock1 = {&solidBlock, 'S', '\0', {470,-150}, {0,0}, 1.75};
	LevelElement level4_ceilingblock2 = {&solidBlock, 'S', '\0', {520,-30}, {0,0}, 1.5};
	LevelElement level4_electrosphere = {&electroSphereAnimated, 'S', 'E', {760,180}, {0,0}, 0.5};
	LevelElement level4_flamethrowerBottom = {&flameContainerU, 'S', '\0', {1080,696}, {0,0}, 1.0};
	LevelElement level4_flamethrowerTop = {&flameContainerD, 'S', '\0', {170,0}, {0,0}, 1.0};
	LevelElement level4_flameBottom = {&flameBurstU, 'S', 'F', {1085,460.5}, {0,0}, 0.88};
	LevelElement level4_flameTop = {&flameBurstD, 'S', 'G', {184,100}, {0,0}, 0.88};
	LevelElement level4_beam = level1_beam1; level4_beam.elementY -= 185;
	LevelElement level4_decoBoard = {&logisticsBoard, 'D', '\0', {1202,220}, {0,0}, 0.44};
	LevelElement level4_potion1 = level1_potion1; 
	level4_potion1.elementY += 100; level4_potion1.elementX += 150;
	LevelElement level4_potion2 = level1_potion2; 
	level4_potion2.elementY += 100; level4_potion2.elementX += 140;
	LevelElement level4_sign = {&sign4, 'D', '\0', {385,594}, {0,0}, 0.65};

	Level level_4 = {
		0.36, // playerSize
		true, true, true, true, // implicit borders
		true, // door locked
		{backgrounda, backgroundb}, // backgrounds 
		{60, 560}, // player coordinates
		{60, 182.5}, // door coordinates
		OFFSCREEN_COORDINATES, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level4_key, level4_electrosphere, level4_ceilingblock2, level4_ceilingblock1, level4_ceiling, level4_platform, level4_stair1, level4_stair2, level4_stair3, level4_stair4, level4_stair5, level4_stair6, level4_flamethrowerBottom, level4_flamethrowerTop, level4_flameBottom, level4_flameTop, level4_beam, level4_decoBoard, level4_potion1, level4_potion2, level4_sign} // elements
	};


	LevelElement level5_shade = level3_shade;
	LevelElement level5_movingPlatform = {&solidPlatform, 'S', '\0', {225,170}, {0,7.5}, 0.8};
	LevelElement level5_leftPlatform = {&solidPlatform, 'S', '\0', {-32,170}, {0,0}, 0.8};
	LevelElement level5_rightPlatform = {&solidPlatform, 'S', '\0', {492,170}, {0,0}, 0.8};
	LevelElement level5_bottomPlatform = {&solidLong, 'S', '\0', {560,680}, {0,0}, 1.0};
	LevelElement level5_block1 = {&metalCrateTall, 'S', '\0', {713,0}, {0,0}, 0.7};
	LevelElement level5_block2 = {&metalCrateTall, 'S', '\0', {1114,212.5}, {0,0}, 0.7};
	LevelElement level5_tutorial = {&tutorialHolo, 'E', '\0', {68,47}, {0,0}, 0.50};
	LevelElement level5_beam1 = {&rsupportBeam, 'E', '\0', {696,170}, {0,0}, 0.5};
	LevelElement level5_verticalBeam = level3_lbeam4; level5_verticalBeam.elementX -= 95;
	LevelElement level5_string1 = {&lsupportString, 'E', '\0', {690,400}, {0,0}, 0.6};
	LevelElement level5_string2 = {&rsupportString, 'E', '\0', {910,400}, {0,0}, 0.6};
	LevelElement level5_doorPlatform = level3_rightPlatform;
	level5_doorPlatform.elementX += 35; level5_doorPlatform.elementY -= 5;
	LevelElement level5_ledge1 = {&semisolidShort, 'S', '\0', {1031,549}, {0,0}, 1.0};
	LevelElement level5_ledge2 = {&semisolidShort, 'S', '\0', {790,359}, {0,0}, 1.0};
	LevelElement level5_electrobeam2 = {&elecBeam, 'S', 'B', {890,215}, {0,0}, 0.7};
	LevelElement level5_electrobeam1 = {&elecBeam, 'S', 'B', {890,380}, {0,0}, 0.7};
	LevelElement level5_lSupportBeam = {&lsupportBeam, 'E', '\0', {54,168}, {0,0}, 0.5};
	LevelElement level5_decoBoard = {&invariantBoard, 'D', '\0', {260, 260}, {0,0}, 0.5};
	LevelElement level5_sign = {&sign5, 'D', '\0', {500,80}, {0,0}, 0.65};

	Level level_5 = {
		0.35, // playerSize
		false, true, true, true, // implicit borders
		false, // door locked
		{backgroundd, backgroundd}, // backgrounds 
		{340, 50}, // player coordinates
		{1313, 47}, // door coordinates
		{607, 109}, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level5_electrobeam1, level5_electrobeam2, level5_ledge1, level5_ledge2, level5_lSupportBeam, level5_doorPlatform, level5_shade, level5_leftPlatform, level5_movingPlatform, level5_rightPlatform, level5_tutorial, level5_bottomPlatform, level5_block1, level5_block2, level5_beam1, level5_verticalBeam, level5_string1, level5_string2, level5_decoBoard, level5_sign} // elements
	};


	LevelElement level6_floatingPlatform = level5_leftPlatform;
	level6_floatingPlatform.elementY += 100;
	LevelElement level6_missilePlatform = level6_floatingPlatform;
	level6_missilePlatform.elementX += 520;
	LevelElement level6_centrePlatform = level6_missilePlatform;
	level6_centrePlatform.elementY = 457; level6_centrePlatform.elementX += 10;
	LevelElement level6_movingPlatform = level2_platform;
	level6_movingPlatform.coordinates = {274,460}; level6_movingPlatform.size = 0.8; level6_movingPlatform.objptr = &semisolidShort;
	level6_floatingPlatform.elementY += 10;
	LevelElement level6_startingBlock = {&solidBlock, 'S', '\0', {-285,460}, {0,0}, 1.22};
	LevelElement level6_pit = {&dmgPlatform, 'S', '\0', {170,642}, {0,0}, 1.06};
	LevelElement level6_bottomWall = {&thinWall, 'S', '\0', {500,476}, {0,0}, 1};
	LevelElement level6_topWall = {&thinWall, 'S', '\0', {722,-323}, {0,0}, 1};
	LevelElement level6_otherBlock = {&solidBlock, 'S', '\0', {862,276}, {0,0}, 1.12};
	LevelElement level6_risingPlatform = level6_movingPlatform;
	level6_risingPlatform.elementX += 500; level6_risingPlatform.elementY -= 100; 
	level6_risingPlatform.velocities = {0,6};
	LevelElement level6_pit2 = {&dmgPlatform, 'S', '\0', {740,475}, {0,0}, 0.66};
	LevelElement level6_flamethrower1 = {&flameContainerD, 'S', '\0', {944,406}, {0,0}, 1.0};
	LevelElement level6_flame1 = {&flameBurstD, 'S', 'F', {960,506}, {0,0}, 0.88};
	LevelElement level6_flamethrower2 = level6_flamethrower1; level6_flamethrower2.elementX += 100;
	LevelElement level6_flame2 = level6_flame1; level6_flame2.elementX += 100;
	LevelElement level6_flamethrower3 = level6_flamethrower2; level6_flamethrower3.elementX += 100;
	LevelElement level6_flame3 = level6_flame2; level6_flame3.elementX += 100;
	LevelElement level6_healthRefill = {&healthRefill, 'S', 'H', {30,180}, {0,0}, 0.5};
	LevelElement level6_missileLauncher1 = {&missileCannon, 'S', 'C', {650,180}, {0,0}, 1};
	LevelElement level6_missileLauncher2 = {&missileCannon, 'S', 'C', {1200,200}, {0,0}, 1};
	LevelElement level6_decoBoard = {&phiBoard, 'D', '\0', {255,30}, {0,0}, 0.5};
	LevelElement level6_sign = {&sign6, 'D', '\0', {145,368}, {0,0}, 0.65};

	Level level_6 = {
		0.36, // playerSize
		true, false, true, true, // implicit borders
		false, // door locked
		{backgrounda, backgroundb}, // backgrounds 
		{60, 360}, // player coordinates
		{600, 544}, // door coordinates
		{597, 408}, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level6_missileLauncher2, level6_missileLauncher1, level6_flamethrower1, level6_flame1, level6_flamethrower2, level6_flame2, level6_flamethrower3, level6_flame3, level6_risingPlatform, level6_pit, level6_startingBlock, level6_startingBlock, level6_floatingPlatform, level6_missilePlatform, level6_centrePlatform, level6_movingPlatform, level6_bottomWall, level6_topWall, level6_pit2, level6_otherBlock, level6_healthRefill, level6_decoBoard, level6_sign} // elements
	};


	LevelElement level7_block1 = {&solidBlock, 'S', '\0', {935,380}, {0,0}, 1};
	LevelElement level7_block2 = {&solidBlock, 'S', '\0', {330,380}, {0,0}, 1};
	LevelElement level7_topPlatform1 = {&semisolidLong, 'S', '\0', {450,100}, {0,0}, 0.7};
	LevelElement level7_topPlatform2 = {&semisolidLong, 'S', '\0', {50,100}, {0,0}, 0.7};
	LevelElement level7_ladder1 = {&semisolidShort, 'S', '\0', {1270,510}, {0,0}, 1.0};
	LevelElement level7_ladder2 = level7_ladder1; 
	LevelElement level7_middlePlatform = {&semisolidPlatform, 'S', '\0', {694,385}, {0,0}, 0.85};
	LevelElement level7_redPlatform1 = {&dmgPlatform, 'S', '\0', {120,200}, {6,0}, 0.76};
	LevelElement level7_redPlatform2 = level7_redPlatform1; level7_redPlatform2.velocities.first *= -1;
	level7_redPlatform2.elementX += 320;
	level7_ladder1.elementY -= 120;
	LevelElement level7_smallPlatform = {&semisolidShort, 'S', '\0', {356,217}, {0,0}, 0.5};
	LevelElement level7_doorPlatform = {&blueSolidPlatform, 'S', '\0', {930,150}, {0,0}, 0.75};
	LevelElement level7_beam1 = {&verticalSupportBeam, 'E', '\0', {410,380}, {0,0}, 0.77};
	LevelElement level7_beam2 = {&verticalSupportBeam, 'E', '\0', {650,380}, {0,0}, 0.77};
	LevelElement level7_beam3 = {&verticalSupportBeam, 'E', '\0', {1000,380}, {0,0}, 0.77};
	LevelElement level7_beam4 = {&verticalSupportBeam, 'E', '\0', {1250,380}, {0,0}, 0.77};
	LevelElement level7_beam5 = {&verticalSupportBeam, 'E', '\0', {1050,150}, {0,0}, 0.9};
	LevelElement level7_beam6 = {&verticalSupportBeam, 'E', '\0', {396,236}, {0,0}, 0.6};
	LevelElement level7_electrosphere = {&electroSphereAnimated, 'S', 'E', {160,10}, {0,0}, 0.4};
	LevelElement level7_string1 = {&supportString, 'E', '\0', {110,-40}, {0,0}, 0.32};
	LevelElement level7_string2 = {&supportString, 'E', '\0', {320,-40}, {0,0}, 0.32};
	LevelElement level7_string3 = {&supportString, 'E', '\0', {520,-40}, {0,0}, 0.32};
	LevelElement level7_string4 = {&supportString, 'E', '\0', {710,-40}, {0,0}, 0.32};
	LevelElement level7_tutorial = {&tutorialHolo, 'E', '\0', {480,590}, {0,0}, 0.50};
	LevelElement level7_decoBoard = {&einsteinBoard, 'D', '\0', {749,430}, {0,0}, 0.45};
	LevelElement level7_shelf = {&tabletop, 'D', '\0', {259.5,460}, {0,0}, 0.6};
	LevelElement level7_potion = {&bluePotion, 'D', '\0', {273,410}, {0,0}, 0.5};
	LevelElement level7_potion2 = {&purplePotion, 'D', '\0', {303,390}, {0,0}, 0.45};
	LevelElement level7_sign = {&sign7, 'D', '\0', {278,595}, {0,0}, 0.65};

	Level level_7 = {
		0.33, // playerSize
		true, false, true, true, // implicit borders
		false, // door locked
		{backgroundc, backgroundc}, // backgrounds 
		{60, 560}, // player coordinates
		{1025, 0}, // door coordinates
		{1090, 642}, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level7_shelf, level7_electrosphere, level7_beam1, level7_beam2, level7_beam3, level7_beam4, level7_beam5, level7_beam6, level7_smallPlatform, level7_ladder1, level7_ladder2, level7_middlePlatform, level7_block1, level7_block2, level7_topPlatform1, level7_topPlatform2, level7_redPlatform1, level7_redPlatform2, level7_doorPlatform, level7_string1, level7_string2, level7_string3, level7_string4, level7_tutorial, level7_decoBoard, level7_potion, level7_potion2, level7_sign} // elements
	};


	LevelElement level8_shade = level3_shade;
	LevelElement level8_electrosphere1 = {&electroSphereAnimated, 'S', 'E', {330,315}, {0,0}, 0.45};
	LevelElement level8_flamethrower1 = {&flameContainerD, 'S', '\0', {350,-50}, {0,0}, 1.0};
	LevelElement level8_flame1 = {&flameBurstD, 'S', 'F', {364,50}, {0,0}, 0.88};
	LevelElement level8_flamethrower2 = level8_flamethrower1;
	level8_flamethrower2.elementX += 650;
	LevelElement level8_flame2 = level8_flame1;
	level8_flame2.elementX += 650; level8_flame2.animCode = 'G';
	LevelElement level8_platform1 = {&semisolidPlatform, 'S', '\0', {30,200}, {0,7}, 0.8};
	LevelElement level8_platform2 = {&semisolidPlatform, 'S', '\0', {470,550}, {0,-7}, 0.8};
	LevelElement level8_platform3 = level8_platform1;
	level8_platform3.elementX += 700;
	LevelElement level8_platform4 = {&semisolidShort, 'S', '\0', {1050,550}, {0,-7}, 0.8};
	LevelElement level8_platform5 = {&semisolidShort, 'S', '\0', {1230,200}, {0,7}, 0.8};
	LevelElement level8_electrosphere2 = level8_electrosphere1;
	level8_electrosphere2.elementX = 960;
	LevelElement level8_beam = level7_beam1;
	level8_beam.coordinates = {592.5,545};
	LevelElement level8_healthRefill = level6_healthRefill;
	level8_healthRefill.coordinates = {712,612};
	LevelElement level8_hiddenPlatform = {&semisolidShort, 'S', '\0', {652,750}, {0,0}, 1.0};
	LevelElement level8_decoBoard = {&sourceCodeBoard, 'D', '\0', {595,27}, {0,0}, 0.6};
	LevelElement level8_sign = {&sign8, 'D', '\0', {35,659}, {0,0}, 0.65};

	Level level_8 = {
		0.38, // playerSize
		false, false, true, true, // implicit borders
		false, // door locked
		{backgrounda, backgrounde}, // backgrounds 
		{70, 50}, // player coordinates
		{1290, 65}, // door coordinates
		{550, 480}, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level8_electrosphere1, level8_electrosphere2, level8_flamethrower1, level8_flame1, level8_flamethrower2, level8_flame2, level8_platform1, level8_platform2, level8_platform3, level8_platform4, level8_platform5, level8_beam, level8_healthRefill, level8_hiddenPlatform, level8_decoBoard, level8_shade, level8_sign} // elements
	};


	LevelElement level9_shade = level8_shade;
	LevelElement level9_missileLauncher1 = {&missileCannon, 'S', 'C', {1176,47}, {0,0}, 1};
	LevelElement level9_missileLauncher2 = {&missileCannon, 'S', 'C', {160,380}, {0,0}, 1};
	LevelElement level9_missileLauncher3 = {&missileCannon, 'S', 'C', {1176,607}, {0,0}, 1};
	LevelElement level9_startingBlock = {&solidBlock, 'S', '\0', {-600,130}, {0,0}, 2};
	LevelElement level9_topBlock = {&solidBlock, 'S', '\0', {1200,-182}, {0,0}, 2};
	LevelElement level9_bottomBlock = {&solidBlock, 'S', '\0', {1200,522}, {0,0}, 2};
	LevelElement level9_topPlatform = {&semisolidPlatform, 'S', '\0', {925,150}, {20,0}, 0.8};
	LevelElement level9_bottomPlatform = {&semisolidPlatform, 'S', '\0', {325,500}, {-20,0}, 0.8};
	LevelElement level9_bridge1 = level1_lowPlatform;
	level9_bridge1.coordinates = {-30,680}; level9_bridge1.size = 0.5;
	LevelElement level9_bridge2 = level9_bridge1;
	level9_bridge2.elementX += 350;
	LevelElement level9_bridge3 = level9_bridge2;
	level9_bridge3.elementX += 350;
	LevelElement level9_bridge4 = level9_bridge3;
	level9_bridge4.elementX += 350;
	LevelElement level9_ledge = level9_bridge1;
	level9_ledge.coordinates = {-50,311};
	LevelElement level9_key = level2_key;
	level9_key.coordinates = {1020,-70};
	LevelElement level9_beam1 = {&verticalSupportBeam, 'E', '\0', {235,680}, {0,0}, 0.77};
	LevelElement level9_beam2 = {&verticalSupportBeam, 'E', '\0', {585,680}, {0,0}, 0.77};
	LevelElement level9_beam3 = {&verticalSupportBeam, 'E', '\0', {935,680}, {0,0}, 0.77};
	LevelElement level9_decoBoard = {&keyBoard, 'D', '\0', {915,27}, {0,0}, 0.4};
	LevelElement level9_sign = {&sign9, 'D', '\0', {156,35}, {0,0}, 0.65};

	Level level_9 = {
		0.34, // playerSize
		false, false, true, true, // implicit borders
		true, // door locked
		{backgroundd, backgroundd}, // backgrounds 
		{40, 10}, // player coordinates
		{1280, 372}, // door coordinates
		{63, 622}, // camera coordinates
		OFFSCREEN_COORDINATES, // simul camera coordinates
		{level9_beam1, level9_beam2, level9_beam3, level9_ledge, level9_bridge1, level9_bridge2, level9_bridge3, level9_bridge4, level9_missileLauncher1, level9_missileLauncher2, level9_missileLauncher3, level9_startingBlock, level9_topBlock, level9_bottomBlock, level9_shade, level9_topPlatform, level9_bottomPlatform, level9_key, level9_decoBoard, level9_sign} // elements
	};


	LevelElement level10_crate = level2_topCrate;
	level10_crate.size += 0.4; level10_crate.elementX -= 120; level10_crate.elementY += 250;
	LevelElement level10_platform1 = {&semisolidShort, 'S', '\0', {10,600}, {0,-7}, 0.8};
	LevelElement level10_platform2 = {&semisolidShort, 'S', '\0', {1210,200}, {0,7}, 0.8};
	LevelElement level10_platform3 = {&semisolidShort, 'S', '\0', {700,175}, {30,0}, 0.8};
	LevelElement level10_tutorial = level7_tutorial;
	level10_tutorial.coordinates = {850,460};
	LevelElement level10_leftLightning1 = {&lightningBeam, 'S', 'L', {200,-20}, {0,0}, 1.9};
	LevelElement level10_leftLightning2 = {&lightningBeam, 'S', 'R', {200,-20}, {0,0}, 1.9};
	LevelElement level10_rightLightning1 = {&lightningBeam, 'S', 'L', {1070,-20}, {0,0}, 1.9};
	LevelElement level10_rightLightning2 = {&lightningBeam, 'S', 'R', {1070,-20}, {0,0}, 1.9};
	LevelElement level10_decoBoard = {&simulBoard, 'D', '\0', {595,194}, {0,0}, 0.6};
	LevelElement level10_sign = {&sign10, 'D', '\0', {655,490}, {0,0}, 0.65};

	Level level_10 = {
		0.38, // playerSize
		true, false, true, true, // implicit borders
		false, // door locked
		{backgrounda, backgroundb}, // backgrounds 
		{700, 460}, // player coordinates
		{678, 28}, // door coordinates
		OFFSCREEN_COORDINATES, // camera coordinates
		{489, 517}, // simul camera coordinates
		{level10_crate, level10_platform1, level10_platform2, level10_platform3, level10_tutorial, level10_leftLightning1, level10_leftLightning2, level10_rightLightning1, level10_rightLightning2, level10_decoBoard, level10_sign} // elements
	};


	LevelElement level11_ceiling1 = level4_ceiling;
	LevelElement level11_ceiling2 = level11_ceiling1;
	level11_ceiling1.elementX -= 1100;
	level11_ceiling2.elementX += 1100;
	LevelElement level11_lowCeiling1 = level11_ceiling1;
	LevelElement level11_lowCeiling2 = level11_ceiling2;
	level11_lowCeiling1.elementX -= 200; level11_lowCeiling1.elementY += 100;
	level11_lowCeiling2.elementX += 200; level11_lowCeiling2.elementY += 100;
	LevelElement level11_floor1 = {&solidBlock, 'S', '\0', {-1118,650}, {0,0}, 3.5};
	LevelElement level11_floor2 = {&solidBlock, 'S', '\0', {1083,650}, {0,0}, 3.5};
	LevelElement level11_highFloor1 = level11_floor1;
	LevelElement level11_highFloor2 = level11_floor2;
	level11_highFloor1.elementX -= 200; level11_highFloor1.elementY -= 100;
	level11_highFloor2.elementX += 200; level11_highFloor2.elementY -= 100;
	level11_floor2.elementX += 350;
	level11_highFloor2.elementX -= 95;
	LevelElement level11_cameraString = {&supportString, 'E', '\0', {273,-4}, {0,0}, 0.22};
	LevelElement level11_electrosphere1 = {&electroSphereAnimated, 'S', 'E', {300,-7}, {0,0}, 0.3};
	LevelElement level11_electrosphere2 = {&electroSphereAnimated, 'S', 'E', {1040,-7}, {0,0}, 0.3};
	LevelElement level11_electrobeam1 = {&elecBeamLong, 'S', 'B', {380,30}, {0,0}, 0.696};
	LevelElement level11_missileLauncher1 = {&missileCannon, 'S', 'C', {531,692}, {0,0}, 1};
	LevelElement level11_missileLauncher2 = {&missileCannon, 'S', 'C', {842,692}, {0,0}, 1};
	LevelElement level11_healthRefill = level6_healthRefill;
	level11_healthRefill.coordinates = {844,600}; level11_healthRefill.size -= 0.1;
	LevelElement level11_platform1 = {&solidShort, 'S', '\0', {360,330}, {0,15}, 0.8};
	LevelElement level11_platform2 = level11_platform1;
	level11_platform2.elementX += 300; level11_platform2.elementY -= 25;
	LevelElement level11_decoBoard = {&brakeBoard, 'D', '\0', {246,288}, {0,0}, 0.65};
	LevelElement level11_sign = {&sign11, 'D', '\0', {75,450}, {0,0}, 0.65};
	LevelElement level11_shade = level3_shade;

	Level level_11 = {
		0.34, // playerSize
		false, false, true, true, // implicit borders
		false, // door locked
		{backgroundd, backgroundd}, // backgrounds 
		{39, 393}, // player coordinates
		{1310, 400}, // door coordinates
		{222, 586.5}, // camera coordinates
		{222, 112}, // simul camera coordinates
		{level11_cameraString, level11_electrobeam1, level11_electrosphere1, level11_electrosphere2, level11_lowCeiling1, level11_lowCeiling2, level11_ceiling1, level11_ceiling2, level11_highFloor1, level11_highFloor2, level11_floor1, level11_floor2, level11_missileLauncher1, level11_missileLauncher2, level11_healthRefill, level11_platform1, level11_platform2, level11_decoBoard, level11_shade, level11_sign} // elements
	};


	LevelElement level12_shade = level3_shade;
	LevelElement level12_startingBlock = {&solidBlock, 'S', '\0', {-1320,135}, {0,0}, 3.5};
	LevelElement level12_floatingBlock = {&solidBlock, 'S', '\0', {375,135}, {0,0}, 1.1};
	LevelElement level12_blockSupport = level12_floatingBlock;
	level12_blockSupport.elementY += 170;
	LevelElement level12_ledge1 = {&semisolidPlatform, 'S', '\0', {660,475}, {0,0}, 0.8};
	LevelElement level12_ledge2 = {&blueSolidPlatform, 'S', '\0', {1200,192}, {0,0}, 0.75};
	LevelElement level12_flamethrowerLeft = {&flameContainerU, 'S', '\0', {390,121}, {0,0}, 1.0};
	LevelElement level12_flameLeft = {&flameBurstU, 'S', 'F', {level12_flamethrowerLeft.elementX+5,level12_flamethrowerLeft.elementY-235.5}, {0,0}, 0.88};
	LevelElement level12_flamethrowerRight = level12_flamethrowerLeft;
	level12_flamethrowerRight.elementX += 330;
	LevelElement level12_flameRight = level12_flameLeft;
	level12_flameRight.elementX += 330;
	LevelElement level12_lowPlatform = {&semisolidPlatform, 'S', '\0', {400,640}, {-10,0}, 0.87};
	LevelElement level12_verticalPlatform1 = {&semisolidShort, 'S', '\0', {975,170}, {0,-9.5}, 0.75};
	LevelElement level12_verticalPlatform2 = {&semisolidShort, 'S', '\0', {1215,590}, {0,9.5}, 0.75};
	LevelElement level12_key = level2_key;
	level12_key.coordinates = {1250,340};
	LevelElement level12_electrosphere1 = level11_electrosphere1;
	level12_electrosphere1.coordinates = {1360,300};
	LevelElement level12_electrosphere2 = level12_electrosphere1;
	level12_electrosphere2.elementY += 340;
	LevelElement level12_leftSupportBeam = level1_beam1;
	level12_leftSupportBeam.elementX += 740; level12_leftSupportBeam.elementY -= 30;
	LevelElement level12_rightSupportBeam = level1_beam2;
	level12_rightSupportBeam.elementY -= 120; level12_rightSupportBeam.elementX += 15;
	LevelElement level12_lowerSupportBeam = level12_leftSupportBeam;
	level12_lowerSupportBeam.elementX -= 130; level12_lowerSupportBeam.elementY += 130;
	LevelElement level12_verticalBeam1 = {&verticalSupportBeam, 'E', '\0', {410,380}, {0,0}, 0.89};
	LevelElement level12_verticalBeam2 = {&verticalSupportBeam, 'E', '\0', {410+185,380}, {0,0}, 0.89};
	LevelElement level12_verticalBeam3 = {&verticalSupportBeam, 'E', '\0', {410+2*185,380}, {0,0}, 0.89};
	LevelElement level12_leftLightning1 = level10_leftLightning1;
	LevelElement level12_leftLightning2 = level10_leftLightning2;
	LevelElement level12_rightLightning1 = level10_rightLightning1;
	LevelElement level12_rightLightning2 = level10_rightLightning2;
	level12_leftLightning1.elementY -= 150; level12_leftLightning1.elementX += 70; level12_leftLightning1.size -= 0.7;
	level12_leftLightning2.elementY -= 150; level12_leftLightning2.elementX += 70; level12_leftLightning2.size -= 0.7;
	level12_rightLightning1.elementY -= 0; level12_rightLightning1.elementX += 60; level12_rightLightning1.size -= 0.3;
	level12_rightLightning2.elementY -= 0; level12_rightLightning2.elementX += 60; level12_rightLightning2.size -= 0.3;
	LevelElement level12_supportString = {&supportString, 'E', '\0', {508,463}, {0,0}, 0.22};
	LevelElement level12_potion = {&bluePotion, 'D', '\0', {497,85}, {0,0}, 0.5};
	LevelElement level12_potion2 = {&purplePotion, 'D', '\0', {527,65}, {0,0}, 0.45};
	LevelElement level12_decoBoard = {&generalBoard, 'D', '\0', {942,33.3}, {0,0}, 0.57};
	LevelElement level12_sign = {&sign12, 'D', '\0', {36,34}, {0,0}, 0.65};

	Level level_12 = {
		0.35, // playerSize
		false, false, true, true, // implicit borders
		true, // door locked
		{backgrounda, backgrounde}, // backgrounds 
		{10,30}, // player coordinates
		{1270, 47}, // door coordinates
		{455, 579}, // camera coordinates
		{575, 73}, // simul camera coordinates
		{level12_supportString, level12_verticalBeam1, level12_verticalBeam2, level12_verticalBeam3, level12_lowerSupportBeam, level12_leftSupportBeam, level12_rightSupportBeam, level12_ledge1, level12_ledge2, level12_flamethrowerLeft, level12_flameLeft, level12_flamethrowerRight, level12_flameRight, level12_startingBlock, level12_floatingBlock, level12_blockSupport, level12_lowPlatform, level12_verticalPlatform1, level12_verticalPlatform2, level12_electrosphere1, level12_electrosphere2, level12_leftLightning1, level12_leftLightning2, level12_rightLightning1, level12_rightLightning2, level12_key, level12_decoBoard, level12_potion, level12_potion2, level12_shade, level12_sign} // elements
	};


	Level levelArray[12] = {level_1, level_2, level_3, level_4, level_5, level_6, level_7, level_8, level_9, level_10, level_11, level_12};

	// Main loop
	
	while (running) 
	{

		switch(gameState)
		{

			case 0: // Game loop
			{
				Mix_Volume(-1,soundVolume); 
				Mix_VolumeMusic(musicVolume);
				startMusic("Gameplay", soundtrack);

				while (SDL_PollEvent(&event)) 
				{

					switch(event.type)
					{

						case SDL_QUIT:
							running = false;
							break;

						case SDL_KEYDOWN: // Handles discrete key presses.
							if (event.key.repeat == 1)
								break;

							for (unsigned int i = 0; i < bodyRenderQueue.size(); i++) {
								if (touching(thePlayer, theBody)) {
									touchingPlatform = true;
									break;
								} else {
									touchingPlatform = false;
								}
							}

							if (!touchingPlatform) {
								for (unsigned int i = 0; i < surfaceRenderQueue.size(); i++) {
									if (touching(thePlayer, theSurface)) {
										touchingPlatform = true;
										break;
									} else {
										touchingPlatform = false;
									}
								}
							} // Checks for being on a falling platform; jumping while on one is otherwise impossible.

							switch(event.key.keysym.sym)
							{
								case SDLK_s:
									//playSound("Level Complete", soundEffects);
		                        	//wait(1);
		                        	//nextLevel = true;
		                        	//goto inputEnd;
		                        	//break;
									// (Previously used for debugging)
								case SDLK_a:
								case SDLK_LEFT:
								case SDLK_d:
								case SDLK_RIGHT:
									playerLengthContraction = 1.0;
									break;
		                    	case SDLK_UP: // Game actions
		                    	case SDLK_w:
		                    		if (grounded || touchingPlatform) {
		                    			playSound("Jump", soundEffects);
		                    			thePlayer.jump(static_cast<int>(165*playerSize));
		                    			grounded = false;
		                    			touchingPlatform  = false;
		                    			landedIndex = -1;
		                    			landedType = 'n';	
		                    		}
		                    		break;	
		                        case SDLK_e:
		                        	if (entityDistance(thePlayer, objectRenderQueue[1]) < 80 && !simulCamera.playerInFrame) { // Player is near relativity camera
		                        		cutsceneCode = relativityOn ? 'D' : 'A';
			                        	relativityOn = !relativityOn;
			                        	train.playerInFrame = !train.playerInFrame;
			                        	camera.playerInFrame = !camera.playerInFrame;
			                        	gameState = 1;
			                        	if (relativityOn) {
			                        		targetTime[1] = timer + 18000;
			                        		targetTime[5] = targetTime[1] - 3000;
			                        	}			                  
		                        	} else if (b < 50) { // Player is near tutorialHolo (Add full cutscenes later)
		                        		stopMusic();
		                        		stopSound();
		                        		playSound("Tutorial", soundEffects);
		                        		switch (currentLevel)
		                        		{
		                        			case 5:
		                        				cutsceneCode = '1';
		                        				gameState = 1;
		                        				window.fadeOut(whiteCover, 150);
		                        				break;
		                        			case 7:
		                        				cutsceneCode = '2';
		                        				gameState = 1;
		                        				window.fadeOut(whiteCover, 150);
		                        				break;
		                        			case 10:
		                        				cutsceneCode = '3';
		                        				gameState = 1;
		                        				window.fadeOut(whiteCover, 150);
		                        				break;
		                        			default:
		                        				break;
		                        		}
		                        	} else if (entityDistance(thePlayer, objectRenderQueue[2]) < 50 && !camera.playerInFrame) { // Player is near other relativity camera
		                        		cutsceneCode = relativityOn ? 'D' : 'A';
			                        	relativityOn = !relativityOn;
			                        	train.playerInFrame = !train.playerInFrame;
			                        	simulCamera.playerInFrame = !simulCamera.playerInFrame;
			                        	gameState = 1;
			                        	if (relativityOn) {
			                        		targetTime[2] = timer + 18000;
			                        		targetTime[5] = targetTime[2] - 3000;
			                        	}	
		                        	} else if (exitDoorOpen) {          		
		                        		playSound("Level Complete", soundEffects);
		                        		restartLevel:
		                        		wait(1);
		                        		nextLevel = true;
		                        		goto inputEnd;
		                        	}
		                        	break;
		                        case SDLK_r:
		                        	if (HP > 0) {
		                        		currentLevel -= 1;
		                        		playSound("Restart", soundEffects);
		                        		goto restartLevel;
		                        	}         
		                        	break;
		                        case SDLK_q:
		                        	if (HP > 0) {
		                        		titleLayer = 'T';
			                        	gameState = 2;
			                        	playSound("Quit to Title", soundEffects);
			                        	window.fadeOut(blackCover, 200);
			                        	wait(1);
			                        	stopMusic();
		                        	}
		                        	break;
		                    	default:
		                        	break;
		                	}
		                	
						default:
							break;
					}

				}
				inputEnd:

				// Player movemement and animation

				SDL_PumpEvents();
				leftPressed = keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A];
				rightPressed = keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D];

				if (landedIndex >= 0 && ((leftPressed && rightPressed) || (!leftPressed && !rightPressed))) {
					thePlayer.setXPrime(surfaceRenderQueue[landedIndex].getXPrime());
					if (surfaceRenderQueue[landedIndex].getYPrime() > 0) {
						thePlayer.setYPrime(surfaceRenderQueue[landedIndex].getYPrime());
					}
					if (thePlayer.getXPrime() == surfaceRenderQueue[landedIndex].getXPrime()) {
						thePlayer.setWidth(playerWidth[3]);
						thePlayer.setHeight(playerHeight[3]);
						thePlayer.setTexture(playerWalk[3]);
						playerLengthContraction = 1.0;
					}
				}

				if (leftPressed && !(rightPressed))
				{
					if (thePlayer.getXPrime() >= -1*maxSpeed) {
						thePlayer.addVelVector(WEST,1);
						facing = false;	
						if (relativityOn)
							playerLengthContraction -= 0.01 * gamma;
					}		

					c++; // lol
					j = static_cast<int>(floor(static_cast<float>((c%1000)/100)));
					thePlayer.setWidth(playerWidth[j]);
					thePlayer.setHeight(playerHeight[j]);
					thePlayer.setTexture(playerWalk[j]);
				}
				if (!(leftPressed) && rightPressed)
				{
					if (thePlayer.getXPrime() <= maxSpeed) {
						thePlayer.addVelVector(EAST,1);
						facing = true;
						if (relativityOn)
							playerLengthContraction -= 0.01 * gamma;
					}

					c++;
					j = static_cast<int>(floor(static_cast<float>((c%1000)/100)));
					thePlayer.setWidth(playerWidth[j]);
					thePlayer.setHeight(playerHeight[j]);
					thePlayer.setTexture(playerWalk[j]);
				}
				if (((leftPressed && rightPressed) || (!leftPressed && !rightPressed)) && (thePlayer.getXPrime() != 0))
				{
					thePlayer.setXPrime(thePlayer.getXPrime() * 0.99);
					if (relativityOn || playerLengthContraction < 1)
							playerLengthContraction *= 1.05;
					if (abs(thePlayer.getXPrime()) < 1 && !iFrame) {
						thePlayer.stopX();
						thePlayer.setWidth(playerWidth[3]);
						thePlayer.setHeight(playerHeight[3]);
						thePlayer.setTexture(playerWalk[3]);
						playerLengthContraction = 1.0;
					}		
				} 


				if (playerLengthContraction > 1.0)
					playerLengthContraction = 1.0;
				if (playerLengthContraction < 0.5)
					playerLengthContraction = 0.5; // Prevents too much warping due to length contraction.

				// Object rendering

				objectRendering:

				window.clear();
				
				for (unsigned int i = 0; i < backgroundRenderQueue.size(); i++) {
					if (relativityOn) {
						dopplerEffect(theBackground, redshiftAmount, blueshiftAmount);
					} else {
						resetColour(theBackground);
					}
					window.renderFullscreen(theBackground);
				}

				for (unsigned int i = 0; i < backgroundObjRenderQueue.size(); i++) {
					if (relativityOn) {
						dopplerEffect(theBackgroundObj, redshiftAmount, blueshiftAmount);
					} else {
						resetColour(theBackgroundObj);
					}
					window.render(theBackgroundObj, backgroundObjRenderSize[i]);
				}

				for (unsigned int i = 0; i < objectRenderQueue.size(); i++) {
					if (relativityOn) {
						dopplerEffect(theObject, redshiftAmount, blueshiftAmount);
					} else {
						resetColour(theObject);
					}
					window.render(theObject, objectRenderSize[i], 1.0, 1.0, false, false, theObject.getTilt());

					if (theObject == door[1] && entityDistance(thePlayer, theObject) <= 220) {
						theObject.setTexture(door[2]);
						playSound("Door Open", soundEffects);
					} else if (theObject == door[2] && entityDistance(thePlayer, theObject) > 220) {
						theObject.setTexture(door[1]);
					} else if (theObject == door[2] && entityDistance(thePlayer, theObject) < 50) {
						exitDoorOpen = true;
					} else if (theObject == door[2] && entityDistance(thePlayer, theObject) > 50) {
						exitDoorOpen = false;
					}

					if (theObject == cameraLaptop[0] && entityDistance(thePlayer, cameraActivator) < 250) {
						theObject.setTexture(cameraLaptop[1]);
					} 
					if (theObject == cameraLaptop[1] && entityDistance(thePlayer, cameraActivator) >= 250) {
						theObject.setTexture(cameraLaptop[0]);
					}

					if (theObject == simulCameraLaptop[0] && entityDistance(thePlayer, simulCameraActivator) < 250) {
						theObject.setTexture(simulCameraLaptop[1]);
					} else if (theObject == simulCameraLaptop[1] && entityDistance(thePlayer, simulCameraActivator) >= 250) {
						theObject.setTexture(simulCameraLaptop[0]);
					}

					if (theObject == tutorialHolo)
					{
						switch(currentLevel)
						{
							case 5:
								theObject.setY(47 + 10*sin(0.001 * timer));
								break;
							case 7:
								theObject.setY(580 + 10*sin(0.001 * timer));
								break;
							case 10:
								theObject.setY(460 + 10*sin(0.001 * timer));			
								break;
							default:
								break;
						}
						b = entityDistance(thePlayer, theObject);
						theObject.setTexture(b < 50 ? tutorialPoint[1] : tutorialPoint[0]);
						setTransparency(theObject, b < 50 ? 195 : 95);
					}

					if (currentLevel != 5 && currentLevel != 7 && currentLevel != 10) {
						b = 0xFFFF;
					}
				}

				for (unsigned int i = 0; i < bodyRenderQueue.size(); i++) {
					if (relativityOn) {
						dopplerEffect(theBody, redshiftAmount, blueshiftAmount);
					} else {
						resetColour(theBody);
					}
					window.render(theBody, bodyRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theBody.getXPrime()))) : 1.0, 1.0);
					theBody.move();
					if (theBody.isBouncy())
						theBody.ifOnEdgeBounce();
				}

				for (unsigned int i = 0; i < surfaceRenderQueue.size(); i++) {
					if (relativityOn) {
						dopplerEffect(theSurface, redshiftAmount, blueshiftAmount);
					} else {
						resetColour(theSurface);
					}

					if (surfaceAnimationCode[i]) {
						switch(surfaceAnimationCode[i])
						{
							case 'B':
								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0, false, timer%2);

								if (currentLevel == 5) {
									if (relativityOn) {
										if (timer%900 == 100) {
											theSurface.toggleVanished();
											if (!theSurface.isVanished())
												playSound("Zap", soundEffects);
										}	
									} else {
										if (timer%300 == 100) {
											theSurface.toggleVanished();
											if (!theSurface.isVanished())
												playSound("Zap", soundEffects);
										}		
									}
								} else if (currentLevel == 11) {
									if (timer%2000 == 100 || timer%2000 == 353) {
										playSound("Zap", soundEffects);
									}	
								}
								
								break;
							case 'C':
								if (currentLevel == 6) {
									window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0, false, false, 180);

									if (timer%4000 == 0) {
										playSound("Missile Shot", soundEffects);
										missile.stop();
										missile.setTilt(0);
										missile.setCoords(level6_missileLauncher1.elementX-70, level6_missileLauncher1.elementY+20);
										missile.addVelVector(WEST,40);
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
									} else if (timer%4000 == 2000) {
										playSound("Missile Shot", soundEffects);
										missile.stop(); 
										missile.setTilt(0); 
										missile.setCoords(level6_missileLauncher2.elementX-70, level6_missileLauncher2.elementY+20);
										missile.addVelVector(WEST,40);
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
									}
								} else if (currentLevel == 9) {
									window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0, false, false, (theSurface.getX() < 900) ? 0 : 180);

									if (timer%5000 == 1000) {
										playSound("Missile Shot", soundEffects);
										missile.stop(); 
										missile.setTilt(0); 
										missile.setCoords(level9_missileLauncher1.elementX-70, level9_missileLauncher1.elementY+20); 
										missile.addVelVector(WEST,40);
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
									} else if (timer%5000 == 2000) {
										playSound("Missile Shot", soundEffects);
										missile.stop(); 
										missile.setTilt(180); 
										missile.setCoords(level9_missileLauncher2.elementX+70, level9_missileLauncher2.elementY+20); 
										missile.addVelVector(EAST,40);
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
									} else if (timer%5000 == 3000) {
										playSound("Missile Shot", soundEffects);
										missile.stop(); 
										missile.setTilt(0); 
										missile.setCoords(level9_missileLauncher3.elementX-70, level9_missileLauncher3.elementY+20); 
										missile.addVelVector(WEST,40);
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
									}
								} else if (currentLevel == 11) {
									window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0, false, false, -90);

									if (simulCamera.playerInFrame) {
										if (timer%3000 == 1) {
											playSound("Missile Shot", soundEffects);
											missile.stop(); 
											missile.setTilt(90); 
											missile.setCoords(level11_missileLauncher1.elementX-20, level11_missileLauncher1.elementY+20);
											missile.addVelVector(SOUTH,40);
											displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
										} else if (timer%3000 == 1502) {
											playSound("Missile Shot", soundEffects);
											missile.stop(); 
											missile.setTilt(90); 
											missile.setCoords(level11_missileLauncher2.elementX-20, level11_missileLauncher2.elementY+20); 
											missile.addVelVector(SOUTH,40);
											displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
										}
									} else {
										if (timer%3000 == 1) {
											playSound("Missile Shot", soundEffects);
											missile.stop(); 
											missile.setTilt(90); 
											missile.setCoords(level11_missileLauncher1.elementX-20, level11_missileLauncher1.elementY+20);
											missile.addVelVector(SOUTH,40);
											displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
										} else if (timer%3000 == 2) {
											playSound("Missile Shot", soundEffects);
											missile.stop(); 
											missile.setTilt(90); 
											missile.setCoords(level11_missileLauncher2.elementX-20, level11_missileLauncher2.elementY+20); 
											missile.addVelVector(SOUTH,40);
											displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile, 1.2, 'M');
										}
									}
								}

								
								break;
							case 'E':
								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0); 
								theSurface.setTexture(electrosphere[((timer/70)%2) + 2]); // Electricity animation.
								break;
							case 'F':
								if (timer == 1) {
									playSound("Flame Burst", soundEffects);
								}

								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0);

								if (timer % 12000 == 0) {
									playSound("Flame Burst", soundEffects);
									theSurface.toggleVanished();
								}
								if (timer % 12000 == 5000) {
									theSurface.toggleVanished();
								}

								if (theSurface == flamethrowerFire[0] || theSurface == flamethrowerFire[1])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 0]);
								if (theSurface == flamethrowerFire[2] || theSurface == flamethrowerFire[3])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 2]);
								if (theSurface == flamethrowerFire[4] || theSurface == flamethrowerFire[5])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 4]);
								if (theSurface == flamethrowerFire[6] || theSurface == flamethrowerFire[7])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 6]); // Flame animation.
								
								break;
							case 'G':
								if (timer == 1) {
									theSurface.toggleVanished();
								}

								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0);

								if (timer % 12000 == 6000) {
									playSound("Flame Burst", soundEffects);
									theSurface.toggleVanished();
								}
								if (timer % 12000 == 11000) {
									theSurface.toggleVanished();
								}

								if (theSurface == flamethrowerFire[0] || theSurface == flamethrowerFire[1])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 0]);
								if (theSurface == flamethrowerFire[2] || theSurface == flamethrowerFire[3])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 2]);
								if (theSurface == flamethrowerFire[4] || theSurface == flamethrowerFire[5])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 4]);
								if (theSurface == flamethrowerFire[6] || theSurface == flamethrowerFire[7])
									theSurface.setTexture(flamethrowerFire[((timer/50)%2) + 6]); // Flame animation.

								break;
							case 'H':
								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0);

								theSurface.changeY(0.005*sin(timer/300.0));

								if (sdlCollided(thePlayer, theSurface)) {
									playSound("Heal", soundEffects);
									removeEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, healthRefill);
									HP = 3;
									targetTime[4] = timer + 2000;
								}
								
								break;
							case 'K':
								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0, false, false, timer/15); // rotates
								if (sdlCollided(thePlayer, theSurface)) {
									playSound("Ding", soundEffects);
									removeEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, exitKey);
									objectRenderQueue[0].setTexture(door[1]);
								}
								break;
							case 'L':
								if (timer == 1) {
									theSurface.vanish();
								}

								if (simulCamera.playerInFrame) {
									if (theSurface.getX() < WINDOW_WIDTH/2) {
										if (timer%4000 == 10) {
											playSound("Lightning", soundEffects);
											theSurface.unvanish();
										} else if (timer%4000 == 2010) {
											theSurface.vanish();
											resetTransparency(theSurface);
										}
									} else if (theSurface.getX() > WINDOW_WIDTH/2) {
										if (timer%4000 == 2010) {
											playSound("Lightning", soundEffects);
											theSurface.unvanish();
										} else if (timer%4000 == 10) {
											theSurface.vanish();
											resetTransparency(theSurface);
										}
									}
								} else {
									if (timer%4000 == 10) {
										playSound("Lightning", soundEffects);
										theSurface.unvanish();
									} else if (timer%4000 == 2010) {
										theSurface.vanish();
										resetTransparency(theSurface);
									}
								}

								if (!theSurface.isVanished()) {
									setTransparency(theSurface, 0xFF - (timer%4000)/8);
								}

								window.render(theSurface, surfaceRenderSize[i], 0.8, 1.0, false, false);
								break;
							case 'M':
								window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0, true, false, theSurface.getTilt());

								if (theSurface != explosion) {
									theSurface.setTexture(missileTextures[(timer/200)%2]);
								} else {
									// ???
								}

								if (theSurface.getX() < -200 || (theSurface.getX() > 1600)) {
									removeEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, missile);
								} else if ((sdlCollided(thePlayer, theSurface) || (currentLevel == 6 && abs(theSurface.getX() - 750) < 10)) && (theSurface != explosion)) {
									theSurface.stop();
									theSurface.setDamage(0);
									theSurface.setTexture(explosion);
									targetTime[3] = timer + 100;
								}
								break;
							case 'R':
								if (timer == 1) {
									theSurface.vanish();
								}

								if (simulCamera.playerInFrame) {
									if (theSurface.getX() < WINDOW_WIDTH/2) {
										if (timer%4000 == 10) {
											playSound("Lightning", soundEffects);
											theSurface.unvanish();
										} else if (timer%4000 == 2010) {
											theSurface.vanish();
											resetTransparency(theSurface);
										}
									} else if (theSurface.getX() > WINDOW_WIDTH/2) {
										if (timer%4000 == 2010) {
											playSound("Lightning", soundEffects);
											theSurface.unvanish();
										} else if (timer%4000 == 10) {
											theSurface.vanish();
											resetTransparency(theSurface);
										}
									}
								} else {
									if (timer%4000 == 2010) {
										playSound("Lightning", soundEffects);
										theSurface.unvanish();
									} else if (timer%4000 == 10) {
										theSurface.vanish();
										resetTransparency(theSurface);
									}
								}

								if (!theSurface.isVanished()) {
									setTransparency(theSurface, 0xFF - (timer%4000)/8);
								}

								window.render(theSurface, surfaceRenderSize[i], 0.8, 1.0, true, false);
								break;
						} // Performs the various obstacle and object animations.
					} else {
						window.render(theSurface, surfaceRenderSize[i], relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0, 1.0);
					}

					theSurface.move();
					if (theSurface.isBouncy())
						theSurface.ifOnEdgeBounce();
					if (currentLevel == 2 && theSurface == solidShort && abs(theSurface.getX()-390) < 0.02)
						theSurface.bounceX();	
					if (currentLevel == 5 && theSurface == solidPlatform && (abs(theSurface.getY()-170) < 0.01 || abs(theSurface.getY()-680) < 0.01))
						theSurface.bounceY();
					if (currentLevel == 6 && theSurface == solidShort) {
						if ((abs(theSurface.getX()-365) < 0.01) || abs(theSurface.getX()-220) < 0.01) {
							theSurface.bounceX();
						} else if ((abs(theSurface.getY()-450) < 0.07) || abs(theSurface.getY()-280) < 0.07) {
							theSurface.bounceY();
						}
					}
					if (currentLevel == 7 && theSurface == dmgPlatform) {
						if (abs(theSurface.getX()-140) < 0.1 || abs(theSurface.getX()-70) < 0.1 || abs(theSurface.getX()-470) < 0.1 || abs(theSurface.getX()-400) < 0.1) {
							theSurface.bounceX();
						}
					}
					if (currentLevel == 8) {
						if ((abs(theSurface.getY()-215) < 0.2 && theSurface.getYPrime() < 0) || (abs(theSurface.getY()-535) < 0.2 && theSurface.getYPrime() > 0)) {
							theSurface.bounceY();
						}
					} 
					if (currentLevel == 9 && theSurface == semisolidPlatform && (abs(theSurface.getX()-226) < 0.5 || abs(theSurface.getX()-955) < 0.5)) {
						theSurface.bounceX();
					}
					if (currentLevel == 10 && theSurface == semisolidShort) {
						if ((theSurface.getXPrime() != 0) && (abs(theSurface.getX()-209) < 0.2 || abs(theSurface.getX()-1025) < 0.2)) {
							theSurface.bounceX();
						} else if (theSurface.getYPrime() < 0 && abs(theSurface.getY()-200) < 0.2) {
							theSurface.bounceY();
						}
					}
					if ((currentLevel == 11 && theSurface == semisolidShort) && (theSurface.getX() < 583 && abs(theSurface.getY()-250) < 0.5)) {
						theSurface.bounceY();
					}
					if (currentLevel == 12) {
						if (theSurface.getYPrime() > 0 && abs(theSurface.getY()-530) < 0.4) {
							theSurface.bounceY();
						} else if (theSurface.getYPrime() < 0 && abs(theSurface.getY()-530) < 0.4) {
							theSurface.bounceY();
						} else if ((abs(theSurface.getX()-141) < 0.4 || abs(theSurface.getX()-800) < 0.4) && theSurface.getYPrime() == 0) {
							theSurface.bounceX();
						}
					} // These dictate where the moving platforms in each level bounce back and forth.
						
				}

				if (playerDied) {
					goto gameEnd;
				}

				if (iFrame || timer < targetTime[4]) {
					health.setTexture(healthBar[HP-1]);
					window.render(health, 0.4);
				}

				// Player rendering

				if (relativityOn) {
					dopplerEffect(thePlayer, redshiftAmount, blueshiftAmount);
				} else {
					resetColour(thePlayer);
				}

				window.render(thePlayer, playerSize, playerLengthContraction, 1.0, !facing); // The player is not contracted in the y direction, because in the train's frame of reference they are only moving at near-light speed in the x direction.
				
				thePlayer.move();

				if (iFrame) {
					setTransparency(thePlayer, 128);
					for (int i = 0; i < 10; i++) {
						SDL_SetTextureAlphaMod(playerWalk[i], 128);
					}
				}

				if (relativityOn) {
					window.renderFullscreen(lens);
					window.renderFullscreen(lensrec); // The camera lens must be rendered after everything else to appear on the top layer.
				}

				if (timer % 500 == 0) {
					lens.toggleVisible();
		            lensrec.toggleVisible();
				}
				
				// Player collision

				for (unsigned int i = 0; i < bodyRenderQueue.size(); i++) {

					if (!bodyHasHitbox[i])
						continue;

					switch(collided(thePlayer, theBody, relativityOn ? playerLengthContraction : 1.0, relativityOn ? 1/((1+abs(0.01*gamma*theBody.getXPrime()))) : 1.0))
					{
						case 1: // right
							thePlayer.move(-1);
							thePlayer.stopX();
							thePlayer.setX(thePlayer.getX()-5);
							break;
						case 3: // left
							thePlayer.move(-1);
							thePlayer.stopX();
							thePlayer.setX(thePlayer.getX()+5);
							break;
						case 2: // top
							thePlayer.move(-1);
							thePlayer.stopY();
							thePlayer.setY(thePlayer.getY()+5);
							thePlayer.jump(0);
							break;
						case 4: // bottom, i.e. landing on object
							thePlayer.move(-1);
							thePlayer.stopY();
							thePlayer.setY(thePlayer.getY()-2);
							grounded = true;
							thePlayer.setWidth(playerWidth[3]);
							thePlayer.setHeight(playerHeight[3]);
							thePlayer.setTexture(playerWalk[3]);
							platformBorderL = theBody.getX();
							platformBorderR = theBody.getX()+(theBody.getWidth()*theBody.getSize());
							platformBorderY = theBody.getY();

							if (theBody.getXPrime() != 0 || theBody.getYPrime() != 0) {
								landedIndex = i;
								landedType = 'b';
							} else {
								landedIndex = -1;
								landedType = 'n';
							} // A nonnegative landedIndex value indicates that the player is on a moving platform, and must accordingly update the platformBorders as the platform moves, until the player leaves it.
							break;
						case 0:
							break;
					}
				}

				for (unsigned int i = 0; i < surfaceRenderQueue.size(); i++) {

					if ((theSurface.getDamage() > 0) && (sdlCollided(thePlayer, theSurface)) && !iFrame) {
						HP -= theSurface.getDamage();
						playSound("Hurt", soundEffects);
						setTransparency(thePlayer, 128);
						for (int i = 0; i < 10; i++) {
							SDL_SetTextureAlphaMod(playerWalk[i], 128);
						}
						iFrame = true;
						targetTime[0] = timer + 2500;
					}
			
					switch(collided(thePlayer, theSurface, relativityOn ? playerLengthContraction : 1.0, relativityOn ? 1/((1+abs(0.01*gamma*theSurface.getXPrime()))) : 1.0))
					{
						case 1: // right
							if (theSurface.isSolid(1)) {
								thePlayer.move(-1);
								thePlayer.stopX();
								thePlayer.setX(thePlayer.getX()-5);
							}
							break;
						case 3: // left
							if (theSurface.isSolid(3)) {
								thePlayer.move(-1);
								thePlayer.stopX();
								thePlayer.setX(thePlayer.getX()+5);
							}					
							break;
						case 2: // top
							if (theSurface.isSolid(2)) {
								thePlayer.move(-1);
								thePlayer.stopY();
								thePlayer.setY(thePlayer.getY()+5);
								thePlayer.jump(0);
								if (iFrame) {
									setTransparency(thePlayer, 128);
									for (int i = 0; i < 10; i++) {
										SDL_SetTextureAlphaMod(playerWalk[i], 128);
									}
								}	
							}
							break;
						case 4: // bottom, i.e. landing on platform or ground
							if (theSurface.isSolid(4)) {
								thePlayer.move(-1);
								thePlayer.stopY();
								thePlayer.setY(thePlayer.getY()-2);
								grounded = true;
								platformBorderL = theSurface.getX();
								platformBorderR = theSurface.getX()+(theSurface.getWidth()*surfaceRenderSize[i]);
								platformBorderY = theSurface.getY();

								if (theSurface.getXPrime() != 0 || theSurface.getYPrime() != 0) {
									landedIndex = i;
									landedType = 's';
								} else {
									landedIndex = -1;
									landedType = 'n';
								}
							}
							break;
						case 0:
							break;
					}
					

					
				}

				if (landedIndex >= 0) {
					switch(landedType)
					{
						case 'b':
							platformBorderL = bodyRenderQueue[landedIndex].getX();
							platformBorderR = bodyRenderQueue[landedIndex].getX()+(bodyRenderQueue[landedIndex].getWidth()*bodyRenderQueue[landedIndex].getSize());
							platformBorderY = bodyRenderQueue[landedIndex].getY();
						case 's':
							platformBorderL = surfaceRenderQueue[landedIndex].getX();
							platformBorderR = surfaceRenderQueue[landedIndex].getX()+(surfaceRenderQueue[landedIndex].getWidth()*surfaceRenderQueue[landedIndex].getSize());
							platformBorderY = surfaceRenderQueue[landedIndex].getY();
					}
				}

				if (grounded && ((thePlayer.getX() < platformBorderL) || (thePlayer.getX() > platformBorderR))) {
					grounded = false;
					thePlayer.jump(0); 
					landedIndex = -1;
					landedType = 'n';
				}

				if (grounded && ((thePlayer.getY()+thePlayer.getHeight()*thePlayer.getSize()+8 < platformBorderY))) {
					grounded = false;
					thePlayer.jump(0); 
					landedIndex = -1;
					landedType = 'n';
				}

				// Player death

				if (thePlayer.getY() > 1000)
					HP--; // void damage
				if (HP == 0) {
					playerDied = true;
					playSound("Game Over", soundEffects);
					setTransparency(thePlayer, 128);
					for (int i = 0; i < 10; i++) {
						SDL_SetTextureAlphaMod(playerWalk[i], 128);
					}

					goto objectRendering; // Dijkstra in shambles
					gameEnd:

					health.setTexture(emptyHealthBar);
					window.render(health, 0.4);
					thePlayer.setWidth(playerWidth[3]);
					thePlayer.setHeight(playerHeight[3]);
					thePlayer.setTexture(playerHurt);

					setTransparency(thePlayer, 128);
					window.render(thePlayer, playerSize, playerLengthContraction, 1.0, !facing);
					window.display();
					wait(2);
					
					do  {
						currentLevel--;
						train.velocity -= 0.005*SPEED_OF_LIGHT;
					} while (currentLevel%3 != 0); // The player gets sent back to level 1, 4, 7, or 10 depending on how far they've progressed.
					playerDied = false;
					HP = 3;
					goto levelGeneration;
				}

				// Timer handling

				if (timer == targetTime[0]) {
					thePlayer.setWidth(playerWidth[3]);
					thePlayer.setHeight(playerHeight[3]);
					thePlayer.setTexture(playerWalk[3]);
					iFrame = false;
					resetTransparency(thePlayer);
					for (int i = 0; i < 10; i++) {
						SDL_SetTextureAlphaMod(playerWalk[i], 255);
					}
				}
				if (timer == targetTime[1] && relativityOn) {
					cutsceneCode = relativityOn ? 'D' : 'A';
                	relativityOn = !relativityOn;
                	train.playerInFrame = !train.playerInFrame;
                	camera.playerInFrame = !camera.playerInFrame;
                	gameState = 1;
				} 
				if (timer == targetTime[2] && relativityOn) {
					cutsceneCode = relativityOn ? 'D' : 'A';
                	relativityOn = !relativityOn;
                	train.playerInFrame = !train.playerInFrame;
                	simulCamera.playerInFrame = !simulCamera.playerInFrame;
                	gameState = 1;
				}
				if (abs(timer - targetTime[3]) < 10) {
					removeEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, kaboom);
				} 
				if (timer == targetTime[5] && relativityOn) {
					playSound("Ticking", soundEffects);
				} 
				if (timer == targetTime[9]) {
					std::cout << a << '\n';
				}

				// Relativity Updating

				if (relativityOn && gameState == 0) {
					gamma = lorentzFactor(train, camera.playerInFrame ? camera : simulCamera);
					redshiftAmount = dopplerShift(dopplerFactor(gamma)).first;
					blueshiftAmount = dopplerShift(dopplerFactor(gamma)).second;
				} else {
					redshiftAmount = 0;
					blueshiftAmount = 0;
				}
				if (train.playerInFrame) {
					camera.playerInFrame = false;
					simulCamera.playerInFrame = false;
				} // In case this gets bugged somehow.

				// Level generation

				if (nextLevel) {
					levelGeneration:
					nextLevel = false;
					window.fadeOut(blackCover, 150);
					window.clear();

					if (currentLevel == 12) {
						cutsceneCode = 'E';
						gameState = 1;
					} else {
						loadLevel(levelArray[currentLevel++], thePlayer, renderQueues, exitDoor, cameraActivator, simulCameraActivator);

						if (levelArray[currentLevel-1].floor)
							displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, floorInvis);
						if (levelArray[currentLevel-1].ceiling)
							displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, ceilingInvis);
						if (levelArray[currentLevel-1].leftWall)
							displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallL);
						if (levelArray[currentLevel-1].rightWall)
							displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallR);

						if (levelArray[currentLevel-1].doorLocked) {
							objectRenderQueue[0].setTexture(door[0]);
						} else {
							objectRenderQueue[0].setTexture(door[1]);
						}

						playerSize = levelArray[currentLevel-1].playerSize;
						relativityOn = false;
						grounded = false;
						facing = true;

						thePlayer.setWidth(playerWidth[3]);
						thePlayer.setHeight(playerHeight[3]);
						thePlayer.setTexture(playerWalk[3]);
						iFrame = false;
						resetTransparency(thePlayer);
						for (int i = 0; i < 10; i++) {
							SDL_SetTextureAlphaMod(playerWalk[i], 255);
						}

						touchingPlatform = false; 
						exitDoorOpen = false;
						platformBorderL = -1000;
						platformBorderR = 3000; 
						platformBorderY = -1000; 
						landedIndex = -1;
						landedType = 'n';
						playerLengthContraction = 1.0;
						b = 100; 
						timer = 0;
						train.velocity += 0.005*SPEED_OF_LIGHT;
						train.playerInFrame = true;
						camera.playerInFrame = false;
						simulCamera.playerInFrame = false;
						for (int i = 0; i <= 4; i++) {
							targetTime[i] = -1;
						}
						window.display();
					}		
				}

			} 

			case 1: // Cutscene
			{
				switch(cutsceneCode)
				{
					case('A'):
						playSound("Activate", soundEffects);
						wait(0.9);
						playSound("Whir", soundEffects);
						cameraStation.setTexture(cameraPlatform[0]);

						for (int i = 1; i <= 3; i++) {
							window.clear();
							window.renderFullscreen(backgrounda);
							window.render(cameraStation, 1.5);
							window.display();
							wait(0.325);
							cameraStation.setTexture(cameraPlatform[i]);
						}

						window.clear();
						window.renderFullscreen(backgrounda);
						window.render(cameraStation, 1.5);
						window.display();
						wait(1.525);
						break;

					case('D'):
						playSound("Deactivate", soundEffects);
						wait(0.9);
						playSound("Whir", soundEffects);
						cameraStation.setTexture(cameraPlatform[3]);

						for (int i = 2; i >= 0; i--) {
							window.clear();
							window.renderFullscreen(backgrounda);
							window.render(cameraStation, 1.5);
							window.display();
							wait(0.325);
							cameraStation.setTexture(cameraPlatform[i]);
						}

						window.clear();
						window.renderFullscreen(backgrounda);
						window.render(cameraStation, 1.5);
						window.display();
						wait(1.525);	
						break;

					case('1'):
						cutsceneContinue = false;
						backgroundt.setTexture(tutorialBG[0]);
						continuePrompt.setCoords(1130, 674); 
						cameraActivator.setCoords(1075, 550); 
						startMusic("Hint", soundtrack);
						window.clear();
						window.renderFullscreen(backgroundt);
						window.display();
						wait(0.5);	

						for (int i = 0; i < 7; i++) {

							switch(i) 
							{
								case 1:
									break;
								case 2:
									backgroundt.setTexture(tutorialBG[1]);
									continuePrompt.changeX(50);
									break;
								case 3:
									break;
								case 4:
									backgroundt.setTexture(tutorialBG[2]);
									continuePrompt.changeX(-30);
									break;
								case 5:
									continuePrompt.changeY(20);
									break;
								case 6:
									break;

							} // Adjusts the display as necessary

							window.clear();
							window.renderFullscreen(backgroundt);
							if (i >= 5)
								window.render(cameraActivator, 0.8);

							for (int j = 1; j <= 255; j++) {
								setTransparency(tutorialTextArray1[i], j);
								window.render(tutorialTextArray1[i], i == 2 ? 0.6 : 1.0);
								window.display();
								wait(0.01);
							}
							while (!cutsceneContinue) {
								while (SDL_PollEvent(&event)) {
									if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
										cutsceneContinue = true;
									}
								}

								window.render(tutorialTextArray1[i], i == 2 ? 0.6 : 1.0);
								window.render(continuePrompt);
								window.display();

								if (cutsceneTimer%100 == 0) {
									continuePrompt.toggleVisible();
								}
								cutsceneTimer++;
							}
							for (int j = 155; j >= 1; j--) {
								window.clear();
								window.renderFullscreen(backgroundt);
								if (i >= 5)
									window.render(cameraActivator, 0.8);
								setTransparency(tutorialTextArray1[i], j);
								window.render(tutorialTextArray1[i], i == 2 ? 0.6 : 1.0);
								window.display();
							}
							cutsceneContinue = false;
							wait(0.25);
						}

						cameraActivator.setCoords(level_5.cameraLocation.first, level_5.cameraLocation.second);
		                stopMusic();
		                stopSound();
						break;

					case('2'):
						cutsceneContinue = false;
						backgroundt.setTexture(tutorialBG[3]);
						continuePrompt.setCoords(1130, 674); 
						startMusic("Hint", soundtrack);
						window.clear();
						window.renderFullscreen(backgroundt);
						window.display();
						wait(0.5);	

						for (int i = 0; i < 3; i++) {

							switch(i) 
							{
								case 1:
									break;
								case 2:
									break;
							} // Adjusts the display as necessary

							window.clear();
							window.renderFullscreen(backgroundt);

							for (int j = 1; j <= 255; j++) {
								setTransparency(tutorialTextArray2[i], j);
								window.render(tutorialTextArray2[i]);
								window.display();
								wait(0.01);
							}
							while (!cutsceneContinue) {
								while (SDL_PollEvent(&event)) {
									if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
										cutsceneContinue = true;
									}
								}

								window.render(tutorialTextArray2[i]);
								window.render(continuePrompt);
								window.display();

								if (cutsceneTimer%100 == 0) {
									continuePrompt.toggleVisible();
								}
								cutsceneTimer++;
							}
							for (int j = 155; j >= 1; j--) {
								window.clear();
								window.renderFullscreen(backgroundt);
								setTransparency(tutorialTextArray2[i], j);
								window.render(tutorialTextArray2[i]);
								window.display();
							}
							cutsceneContinue = false;
							wait(0.25);
						}
	
		                stopMusic();
		                stopSound();
						break;

					case('3'):
						cutsceneContinue = false;
						backgroundt.setTexture(tutorialBG[4]);
						continuePrompt.setCoords(1130, 674);
						simulCameraActivator.setCoords(1205, 630);
						startMusic("Hint", soundtrack);
						window.clear();
						window.renderFullscreen(backgroundt);
						window.display();
						wait(0.5);	

						for (int i = 0; i < 4; i++) {

							switch(i) 
							{
								case 0:
									continuePrompt.changeX(-60);
									continuePrompt.changeY(25);
									break;
								case 1:
									break;
								case 2:
									break;
								case 3:
									break;
							} // Adjusts the display as necessary

							window.clear();
							window.renderFullscreen(backgroundt);
							if (i >= 2)
								window.render(simulCameraActivator, 0.52);

							for (int j = 1; j <= 255; j++) {
								setTransparency(tutorialTextArray3[i], j);
								window.render(tutorialTextArray3[i]);
								window.display();
								wait(0.01);
							}
							while (!cutsceneContinue) {
								while (SDL_PollEvent(&event)) {
									if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) {
										cutsceneContinue = true;
									}
								}

								window.render(tutorialTextArray3[i]);
								window.render(continuePrompt);
								window.display();

								if (cutsceneTimer%100 == 0) {
									continuePrompt.toggleVisible();
								}
								cutsceneTimer++;
							}
							for (int j = 155; j >= 1; j--) {
								window.clear();
								window.renderFullscreen(backgroundt);
								if (i >= 2)
									window.render(simulCameraActivator, 0.52);
								setTransparency(tutorialTextArray3[i], j);
								window.render(tutorialTextArray3[i]);
								window.display();
							}
							cutsceneContinue = false;
							wait(0.25);
						}

						simulCameraActivator.setCoords(level_10.simulCameraLocation.first, level_10.simulCameraLocation.second);
		                stopMusic();
		                stopSound();
						break;

					case('O'):
						resetColour(cutsceneBG);
						cutscenePlayer.setWidth(playerWidth[3]);
						cutscenePlayer.setHeight(playerHeight[3]);
						cutscenePlayer.setTexture(playerWalk[3]);
						cutscenePlayer.setCoords(-100, 620);
						cutsceneMiddleCar2.setCoords(CENTER.first+275, CENTER.second);
						cutsceneMiddleCar1.setCoords(CENTER.first-275, CENTER.second);
						cutsceneMiddleCar0.setCoords(CENTER.first-275*2, CENTER.second);
						cutsceneRearCar.setCoords(CENTER.first-275*3 + 38, CENTER.second);
						cutscenePlayerCar.setCoords(CENTER.first, CENTER.second);
						for (int s = 0; s < 14; s++) {
								sideTracks[s].setCoords(684*(s-1), 530);
						}

						startMusic("Opening Cutscene", soundtrack);
						cutsceneBG.setTexture(skyBG);
						window.renderFullscreen(cutsceneBG);
						window.display();
						wait(1);

						for (int i = 1; i <= 255; i++) {
							setTransparency(date, i);
							window.render(date);
							window.display();
							wait(0.01);
						}
						wait(1);
						for (int i = 255; i >= 0; i--) {
							window.clear();
							window.renderFullscreen(cutsceneBG);
							setTransparency(date, i);
							window.render(date);
							window.display();
						}

						repeat(3000) {
							clouds.changeY(-0.5);
							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(clouds, 1.4);
							window.display();
							SDL_PumpEvents(); // This prevents crashing by reminding SDL that the program is still running.
						}

						for (int i = 1; i <= 255; i++) {
							setTransparency(cutsceneBG2, i);
							setTransparency(station, i);
							window.renderFullscreen(cutsceneBG);
							window.renderFullscreen(cutsceneBG2);
							window.render(station, 1.1);
							window.display();
							wait(0.004);
							SDL_PumpEvents();
						}

						cutscenePlayer.setY(530);
						repeat (120) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d++; d %= 10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(14);
							if (i >= 100)
								cutscenePlayer.hide();

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.renderFullscreen(cutsceneBG2);
							window.render(station, 1.1);
							window.render(cutscenePlayer, 0.55);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}

						cutscenePlayer.show();
						cutsceneBG.setTexture(stationBG);
						cutscenePlayer.setCoords(-100, 460);
						repeat (83) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d++; d %= 10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(11);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutscenePlayer, 0.40);
							window.render(cutsceneFrontTrain, 1.3);
							window.render(frontTracks, 0.35, 0.9);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}

						cutscenePlayer.setWidth(playerWidth[3]);
						cutscenePlayer.setHeight(playerHeight[3]);
						cutscenePlayer.setTexture(playerWalk[3]);
						cutsceneFrontTrain.setTexture(frontFacingTrain[1]);
						cutsceneFrontTrain.setWidth(203);
						cutsceneFrontTrain.changeX(-(203-159) - 10);
						window.clear();
						window.renderFullscreen(cutsceneBG);
						window.render(cutscenePlayer, 0.40);
						window.render(cutsceneFrontTrain, 1.3);
						window.render(staircase, 1, 1, 0.60);
						window.render(frontTracks, 0.35, 0.9);
						window.display();
						playSound("Train Whistle", soundEffects);
						wait(2);

						repeat (30) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d++; d %= 10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(11);
							if (i >= 12)
								cutscenePlayer.changeY(-11);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutscenePlayer, 0.40);
							window.render(cutsceneFrontTrain, 1.3);
							window.render(frontTracks, 0.35, 0.9);
							window.render(staircase, 1, 1, 0.60);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
							if (i == 29)
								cutscenePlayer.hide();
							SDL_PumpEvents();
						}

						window.fadeOut(blackCover, 85);
						playSound("Train Accelerate", soundEffects);
						repeat (16) {
							musicVolume--;
							Mix_VolumeMusic(musicVolume);
							wait(0.1875);
						}
						stopMusic();
						wait(17);
						repeat (32) {
							soundVolume--;
							Mix_Volume(-1,soundVolume); 
							wait(0.125);
						}
						stopSound();
						wait(1);

						soundVolume = (soundToggle == soundButton[1]) ? 0 : 32;
						Mix_Volume(-1,soundVolume); 
						playSound("Space Ambience", soundEffects);
						resetColour(cutsceneBG);
						cutsceneBG.setTexture(indoorBackground);
						cutscenePlayer.setWidth(playerSW);
						cutscenePlayer.setHeight(playerSH);
						cutscenePlayer.setTexture(playerSleep);
						cutscenePlayer.setCoords(75, 600);
						for (int i = 1; i <= 255; i++) {
							setTransparency(cutsceneBG, i);
							setTransparency(bed, i);
							setTransparency(cutscenePlayer, i);
							window.renderFullscreen(cutsceneBG);
							window.render(bed);
							window.render(cutscenePlayer, 0.55);
							window.display();
							wait(0.003);
							SDL_PumpEvents();
						}
						wait(0.75);

						cutscenePlayer.setWidth(playerWidth[3]);
						cutscenePlayer.setHeight(playerHeight[3]);
						cutscenePlayer.setTexture(playerWalk[3]);
						cutscenePlayer.changeY(-92);
						window.clear();
						window.renderFullscreen(cutsceneBG);
						window.render(bed);
						window.render(cutscenePlayer, 0.55);
						window.display();
						wait(0.8);

						repeat (170) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d = (d+1)%10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(11);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(bed);
							window.render(cutscenePlayer, 0.55);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}

						cutscenePlayer.setX(-50);
						repeat (78) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d = (d+1)%10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(11);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutsceneBoard, 0.85);
							window.render(table, 0.7);
							window.render(cutscenePlayer, 0.55);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}
						wait(1.5);
						repeat (78) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d = (d+1)%10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(11);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutsceneBoard, 0.85);
							window.render(table, 0.7);
							window.render(cutscenePlayer, 0.55);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}

						cutscenePlayer.setX(-50);
						cutsceneBG.setTexture(backgrounda.getTexture());
						cutsceneBG2.setTexture(backgroundb.getTexture());
						repeat (80) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d = (d+1)%10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(10);
							if (i >= 50 && i <= 60)
								cutscenePlayer.changeY(-8);
							if (i == 61)
								cutscenePlayer.changeY(-12);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.renderFullscreen(cutsceneBG2);
							window.render(elevatedPlatform, 0.50);
							window.render(cutscenePlayer, 0.55);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}
						wait(1);

						cutscenePlayer.setWidth(playerLW);
						cutscenePlayer.setHeight(playerLH);
						cutscenePlayer.setTexture(playerLook);
						cutscenePlayer.changeY(-15);
						window.clear();
						window.renderFullscreen(cutsceneBG);
						window.renderFullscreen(cutsceneBG2);
						window.render(elevatedPlatform, 0.50);
						window.render(cutscenePlayer, 0.55);
						window.display();
						wait(1);

						cutsceneBG.setTexture(galaxyBG);
						playSound("Realization", soundEffects);
						repeat (300) {
							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutscenePlayerCar);
							window.render(cutsceneMiddleCar0);
							window.render(cutsceneMiddleCar1);
							window.render(cutsceneMiddleCar2);
							window.render(cutsceneMiddleCar3);
							window.render(cutsceneRearCar);
							for (int s = 0; s < 14; s++) {
								window.render(sideTracks[s], 0.8);
								sideTracks[s].changeX(-20);
							}
							window.display();
							wait(0.01);
						}

						window.fadeOut(blackCover,2500);
						wait(2);
						stopSound();
						musicVolume = Mix_PausedMusic() ? 0 : 16;
						soundVolume = (soundToggle == soundButton[1]) ? 0 : 32;
						goto startGame;
						break;

					case('E'):
						cutscenePlayer.setWidth(playerWidth[3]);
						cutscenePlayer.setHeight(playerHeight[3]);
						cutscenePlayer.setTexture(playerWalk[3]);
						cutscenePlayer.setCoords(-860, 150);
						lever.setCoords(750, 557);
						cutsceneSideTrain.setCoords(CENTER.first-180, CENTER.second-147);
						cutsceneMiddleCar1.setCoords(CENTER.first-275, CENTER.second);
						cutsceneMiddleCar0.setCoords(CENTER.first-275*2, CENTER.second);
						cutsceneMiddleCarn1.setCoords(CENTER.first-275*3, CENTER.second);
						cutsceneMiddleCarn2.setCoords(CENTER.first-275*4, CENTER.second);
						electroSphere.setCoords(958, 270);
						for (int i = 1; i <= 3; i++) {
							electroSphere.setTexture(electrosphere[i]);
							resetColour(electroSphere);
						}		
						for (int i = 0; i < 10; i++) {
							thePlayer.setTexture(playerWalk[i]);
							resetColour(thePlayer);
						}

						stopMusic();
						if (soundVolume != 0) {
							soundVolume /= 2;
							Mix_Volume(-1,soundVolume);
						}
						startMusic("Ending Cutscene", soundtrack);
						playSound("Space Ambience", soundEffects);
						cutsceneBG.setTexture(indoorBackground);
						window.renderFullscreen(cutsceneBG);
						window.render(cutscenePlayer, 0.55);
						window.render(crate, 0.8);
						window.render(crate2, 0.8);
						window.render(crate3, 0.8);
						window.render(factoryBarrier, 1.2);
						window.render(factoryBarrier2, 1.2);
						window.render(lever, 1.5);
						window.render(miniWindow, 2.2);
						window.render(electroSphere, 0.8);
						window.display();

						repeat (116) {
							if (static_cast<int>(cutscenePlayer.getX()) % 3 == 0) {
								d++; d %= 10;
								cutscenePlayer.setWidth(playerWidth[d]);
								cutscenePlayer.setHeight(playerHeight[d]);
								cutscenePlayer.setTexture(playerWalk[d]);
							}
							cutscenePlayer.changeX(13);
							if (i == 85) {
								cutscenePlayer.changeY(220);
							} else if (i == 110) {
								cutscenePlayer.changeY(140);
							} else if (i == 116) {
								cutscenePlayer.setWidth(playerWidth[3]);
								cutscenePlayer.setHeight(playerHeight[3]);
								cutscenePlayer.setTexture(playerWalk[3]);
							}
							electroSphere.setTexture(electrosphere[abs(static_cast<int>(cutscenePlayer.getX()))%3 + 1]);

							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutscenePlayer, 0.55);
							window.render(crate, 0.8);
							window.render(crate2, 0.8);
							window.render(crate3, 0.8);
							window.render(factoryBarrier, 1.2);
							window.render(factoryBarrier2, 1.2);
							window.render(lever, 1.5);
							window.render(miniWindow, 2.2);
							window.render(electroSphere, 0.8);
							window.display();
							wait(0.03);
							SDL_PumpEvents();
						}

						wait(0.2);
						lever.setTexture(bgLever[1]);
						electroSphere.setTexture(electrosphere[0]);
						lever.changeX(-60);
						cutscenePlayer.changeX(-40);

						window.clear();
						window.renderFullscreen(cutsceneBG);
						window.render(cutscenePlayer, 0.55);
						window.render(crate, 0.8);
						window.render(crate2, 0.8);
						window.render(crate3, 0.8);
						window.render(factoryBarrier, 1.2);
						window.render(factoryBarrier2, 1.2);
						window.render(lever, 1.5);
						window.render(miniWindow, 2.2);
						electroSphere.changeX(20);
						electroSphere.changeY(10);
						window.render(electroSphere, 0.6);
						window.display();
						wait(0.03);
						SDL_PumpEvents();

						playSound("Engine Shutdown", soundEffects);
						wait(2);


						d = 0;
						trainFrameDelay = 0.03;
						cutsceneBG.setTexture(galaxyBG);
						repeat (30) {
							window.clear();
							window.renderFullscreen(cutsceneBG);
							window.render(cutsceneMiddleCarn2);
							window.render(cutsceneMiddleCarn1);
							window.render(cutsceneMiddleCar0);
							window.render(cutsceneMiddleCar1);
							window.render(cutsceneSideTrain);

							for (int s = 0; s < 14; s++) {
								window.render(sideTracks[s], 0.8);
							}
							window.display();

							d = (d+1)%8;
							cutsceneSideTrain.setTexture(trainFrames[d]);
							cutsceneSideTrain.changeX(1.0/trainFrameDelay);
							cutsceneMiddleCar1.changeX(1.0/trainFrameDelay);
							cutsceneMiddleCar0.changeX(1.0/trainFrameDelay);
							cutsceneMiddleCarn1.changeX(1.0/trainFrameDelay);
							cutsceneMiddleCarn2.changeX(1.0/trainFrameDelay);

							wait(trainFrameDelay);
							trainFrameDelay += 0.005;
						}
						wait(1.5);
						window.fadeOut(blackCover, 100);
						cutsceneBG.setTexture(window.loadTexture("res/gfx/miscellaneous/Black.png"));

						for (int i = 1; i <= 255; i++) {
							setTransparency(tbc, i);
							window.render(tbc);
							window.display();
							wait(0.01);
						}
						repeat (16) {
							musicVolume--;
							Mix_VolumeMusic(musicVolume);
							wait(0.1875);
						}
						wait(0.5);
						for (int i = 255; i >= 0; i--) {
							window.clear();
							window.renderFullscreen(cutsceneBG);
							setTransparency(tbc, i);
							window.render(tbc);
							window.display();
						}
						wait(2);
						titleLayer = 'T';
                    	gameState = 2;
                    	wait(1);
                    	stopMusic();
						stopSound();
						musicVolume = Mix_PausedMusic() ? 0 : 16;
						soundVolume = (soundToggle == soundButton[1]) ? 0 : 32;
						break;

					default:
						break;
				}

				cutsceneCode = 'N';
				if (gameState == 1)
					gameState = 0;
				break;
	
			}

			case 2: // Title Screen
			{
				Mix_Volume(-1,soundVolume); 
				Mix_VolumeMusic(musicVolume);
				startMusic("Title Screen", soundtrack);
				while (SDL_PollEvent(&event)) 
				{

					switch(event.type)
					{

						case SDL_QUIT:
							running = false;
							break;

		                case SDL_MOUSEMOTION: // Mouse handling
		                	SDL_GetMouseState(&mouseX, &mouseY);
		                	break;

		                case SDL_MOUSEBUTTONDOWN:
		                	if (event.key.repeat == 1)
								break;

		                	if (mouseOver(play, mouseX, mouseY) && titleLayer == 'T')
		                		titleLayer = 'P';
		                	if (mouseOver(newGame, mouseX, mouseY) && titleLayer == 'P') {
		                		playSound("Star Shine", soundEffects);
		                		window.fadeOut(whiteCover, 50);
		                		wait(0.05);
		                		cutsceneCode = 'O';
		                		gameState = 1;
		                		stopMusic();

		                		if (false) { // this block can only be goto-ed using the startGame label.
		                			startGame:
		                			currentLevel = 1;

		                			loadLevel(levelArray[currentLevel-1], thePlayer, renderQueues, exitDoor, cameraActivator, simulCameraActivator);

									if (levelArray[currentLevel-1].floor)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, floorInvis);
									if (levelArray[currentLevel-1].ceiling)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, ceilingInvis);
									if (levelArray[currentLevel-1].leftWall)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallL);
									if (levelArray[currentLevel-1].rightWall)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallR);

									objectRenderQueue[0].setTexture(door[levelArray[currentLevel-1].doorLocked ? 0 : 1]);

									playerSize = levelArray[currentLevel-1].playerSize;
									grounded = false;
									facing = true;
									iFrame = false; 
									touchingPlatform = false; 
									exitDoorOpen = false;
									platformBorderL = -1000;
									platformBorderR = 3000; 
									platformBorderY = -1000; 
									landedIndex = -1;
									landedType = 'n'; 
									timer = 0;
									window.display();

		                			gameState = 0;
		                			cutsceneCode = 'N';
		                			stopMusic();
		                		}
		                		
		                	}	
		                	if (mouseOver(levelSelect, mouseX, mouseY) && titleLayer == 'P')
		                		titleLayer = 'L';
		                	if (mouseOver(controls, mouseX, mouseY) && titleLayer == 'T')
		                		titleLayer = 'C';
		                	if (mouseOver(credits, mouseX, mouseY) && titleLayer == 'T')
		                		titleLayer = 'R';
		                	if (mouseOver(back, mouseX, mouseY) && titleLayer != 'T') 
		                		titleLayer = (titleLayer == 'L') ? 'P' : 'T';
		                	if (mouseOver(musicToggle, mouseX, mouseY) && titleLayer == 'T') {
		                		toggleMusic();
		                		musicToggle.setTexture(musicButton[Mix_PausedMusic()]);
		                		musicVolume = Mix_PausedMusic() ? 0 : 16;
		                	}
		                	if (mouseOver(soundToggle, mouseX, mouseY) && titleLayer == 'T') {
		                		soundVolume = (soundVolume == 0) ? 32 : 0;
		                		soundToggle.setTexture(soundButton[soundVolume == 0]);
		                	}

		                	for (int i = 0; i < 12; i++) {
		                		if (mouseOver(levels[i], mouseX, mouseY) && titleLayer == 'L') {
		                			currentLevel = ++i;

		                			loadLevel(levelArray[currentLevel-1], thePlayer, renderQueues, exitDoor, cameraActivator, simulCameraActivator);

									if (levelArray[currentLevel-1].floor)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, floorInvis);
									if (levelArray[currentLevel-1].ceiling)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, ceilingInvis);
									if (levelArray[currentLevel-1].leftWall)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallL);
									if (levelArray[currentLevel-1].rightWall)
										displayEntity(&surfaceRenderQueue, &surfaceRenderSize, &surfaceAnimationCode, wallR);
									
									objectRenderQueue[0].setTexture(door[levelArray[currentLevel-1].doorLocked ? 0 : 1]);

									playerSize = levelArray[currentLevel-1].playerSize;
									grounded = false;
									facing = true;
									iFrame = false; 
									touchingPlatform = false; 
									exitDoorOpen = false;
									platformBorderL = -1000;
									platformBorderR = 3000; 
									platformBorderY = -1000; 
									landedIndex = -1;
									landedType = 'n'; 
									timer = 0;
									window.display();

		                			gameState = 0;
		                			stopMusic();
		                		}
		                	}

		                	break;

						default:
							break;
					}

				}

				window.renderFullscreen(titleBackground);
				if (titleLayer != 'T') {
					window.render(back, mouseOver(back, mouseX, mouseY) ? 1.4 : 1.2);
				}

				switch(titleLayer) // Menu button rendering
				{
					case 'T':
						window.render(title, 1);
						title.setY(60 + 10*sin(0.001 * timer));

						window.render(play, mouseOver(play, mouseX, mouseY) ? 1.3 : 1.15);
						window.render(controls, mouseOver(controls, mouseX, mouseY) ? 1.3 : 1.15);
						window.render(credits, mouseOver(credits, mouseX, mouseY) ? 1.3 : 1.15);
						window.render(musicToggle, mouseOver(musicToggle, mouseX, mouseY) ? 1.0 : 0.85);
						window.render(soundToggle, mouseOver(soundToggle, mouseX, mouseY) ? 1.0 : 0.85);
						
						break;
					case 'P':
						window.render(newGame, mouseOver(newGame, mouseX, mouseY) ? 1.4 : 1.25);
						window.render(levelSelect, mouseOver(levelSelect, mouseX, mouseY) ? 1.4 : 1.25);
						break;
					case 'L':
						for (int i = 0; i < 12; i++) {
							window.render(levels[i], mouseOver(levels[i], mouseX, mouseY) ? 1.55 : 1.25);
						}	
						break;
					case 'C':
						window.render(controlsList, 1);
						break;
					case 'R':
						window.render(creditList, 1);
						break;
					default:
						break;
				}	

				SDL_PumpEvents();

				break;
			}

		}

		window.display();
		wait(relativityOn ? 90*gamma*(1.0/tickRate) : (1.0/tickRate)); // Time dilation.
		timer++;

	}
	window.cleanUp();
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();
	return 0;
}