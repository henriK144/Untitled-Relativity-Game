#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

using std::string;
using std::abs;
using std::pair;
using std::vector;
using std::map;

namespace gamefuncs {
	void wait(float s);
	bool percentChance(int p);

	void debug(float m);
	void debug(int m);
	void debug(Uint8 m);
	void debug(bool m);
	void debug(char m);
	void debug(string m);
	void displayText(string text, float delay);

	void displayEntity(vector<Surface>* queue, vector<float>* sizeQueue, vector<char>* animQueue, Surface s, float size=1.0, char animCode='\0');
	void displayEntity(vector<Body>* queue, vector<float>* sizeQueue, vector<bool>* hitboxQueue, Body b, float size=1.0, bool hitbox=false);
	void displayEntity(vector<Entity>* queue, vector<float>* sizeQueue, Entity e, float size=1.0);

	void removeEntity(vector<Surface>* queue, vector<float>* sizeQueue, vector<char>* animQueue, Entity sprite);
	void removeEntity(vector<Body>* queue, vector<float>* sizeQueue, vector<bool>* hitboxQueue, Entity sprite);
	void removeEntity(vector<Entity>* queue, vector<float>* sizeQueue, Entity sprite);

	void clearEntities(vector<Surface>* queue, vector<float>* sizeQueue, vector<char>* animQueue);
	void clearEntities(vector<Body>* queue, vector<float>* sizeQueue, vector<bool>* hboxQueue);
	void clearEntities(vector<Entity>* queue, vector<float>* sizeQueue);

	int collisionDetected(SDL_Rect a, SDL_Rect b);
	int collided(Entity e, Entity f, float eContraction=1.0, float fContraction=1.0);
	bool sdlCollided(Entity e, Entity f);
	bool touching(Entity playerHere, Entity platformHere);
	bool mouseOver(Entity e, int mX, int mY); 

	float distance(float x1, float x2, float y1, float y2);
	float entityDistance(Entity e, Entity f);

	void setColour(Entity e, Uint8 r, Uint8 g, Uint8 b);
	void dopplerEffect(Entity e, Uint8 redshift, Uint8 blueshift);
	void resetColour(Entity e);
	Uint8 getTransparency(Entity e, Uint8* p);
	void setTransparency(Entity e, Uint8 a);
	void resetTransparency(Entity e);

	void playSound(string soundKey, map<string, Mix_Chunk*>& soundMap, int r=0);
	void stopSound();
	void startMusic(string musicKey, map<string, Mix_Music*>& musicMap);
	void toggleMusic();
	void stopMusic();
} // Contains several functions for use with entities, and to control other aspects of the game. See gamefuncs.cpp for descriptions.