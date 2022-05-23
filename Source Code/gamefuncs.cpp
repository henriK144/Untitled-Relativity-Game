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

using std::string;
using std::abs;
using std::pair;
using std::vector;
using std::map;



void gamefuncs::wait(float s)
{
	Sleep(static_cast<int>(1000 * s));
	SDL_PumpEvents();
} // s is in seconds.

bool gamefuncs::percentChance(int p)
{
	return (p >= (rand() % 100) + 1);
} // Has a p percent chance of returning true. Currently unused.

void gamefuncs::debug(float m) 
{
	std::cout << m << '\n';
} // "Debugs" using print statements.
void gamefuncs::debug(int m) 
{
	std::cout << m << '\n';
} 
void gamefuncs::debug(Uint8 m) 
{
	std::cout << m << '\n';
} 
void gamefuncs::debug(bool m) 
{
	std::cout << m << '\n';
} // Overloading lets the function debug several different types.
void gamefuncs::debug(char m) 
{
	std::cout << m << '\n';
} 
void gamefuncs::debug(string m) 
{
	std::cout << m << '\n';
} 

void gamefuncs::displayEntity(vector<Surface>* queue, vector<float>* sizeQueue, vector<char>* animQueue, Surface s, float size, char animCode) 
{
	queue->push_back(s);
	sizeQueue->push_back(size);
	animQueue->push_back(animCode);
}
void gamefuncs::displayEntity(vector<Body>* queue, vector<float>* sizeQueue, vector<bool>* hitboxQueue, Body b, float size, bool hitbox) 
{
	queue->push_back(b);
	sizeQueue->push_back(size);
	hitboxQueue->push_back(hitbox);
}
void gamefuncs::displayEntity(vector<Entity>* queue, vector<float>* sizeQueue, Entity e, float size) 
{
	queue->push_back(e);
	sizeQueue->push_back(size);
} // Displays an entity by adding it to the render queue.

void gamefuncs::removeEntity(vector<Surface>* queue, vector<float>* sizeQueue, vector<char>* animQueue, Entity sprite)
{
	for (unsigned int i = 0; i < queue->size(); i++) {
		if (queue->at(i) == sprite) {
			queue->erase(queue->begin() + i);
			sizeQueue->erase(sizeQueue->begin() + i);
			animQueue->erase(animQueue->begin() + i);
			break;
		}
	}	
}
void gamefuncs::removeEntity(vector<Body>* queue, vector<float>* sizeQueue, vector<bool>* hitboxQueue, Entity sprite)
{
	for (unsigned int i = 0; i < queue->size(); i++) {
		if (queue->at(i) == sprite) {
			queue->erase(queue->begin() + i);
			sizeQueue->erase(sizeQueue->begin() + i);
			hitboxQueue->erase(hitboxQueue->begin() + i);
			break;
		}
	}
}
void gamefuncs::removeEntity(vector<Entity>* queue, vector<float>* sizeQueue, Entity sprite)
{
	for (unsigned int i = 0; i < queue->size(); i++) {
		if (queue->at(i) == sprite) {
			queue->erase(queue->begin() + i);
			sizeQueue->erase(sizeQueue->begin() + i);
			break;
		}
	}
} // Deletes an entity by removing it from the render queue. Pass the Entity to be removed.

void gamefuncs::clearEntities(vector<Surface>* queue, vector<float>* sizeQueue, vector<char>* animQueue) 
{
	queue->clear();
	sizeQueue->clear();
	animQueue->clear();
}
void gamefuncs::clearEntities(vector<Body>* queue, vector<float>* sizeQueue, vector<bool>* hboxQueue) 
{
	queue->clear();
	sizeQueue->clear();
	hboxQueue->clear();
}
void gamefuncs::clearEntities(vector<Entity>* queue, vector<float>* sizeQueue) 
{
	queue->clear();
	sizeQueue->clear();
} // Clears all entities from the queue.

int gamefuncs::collisionDetected(SDL_Rect a, SDL_Rect b)
{
    int leftA, leftB, rightA, rightB, topA, topB, bottomA, bottomB;
    int v = 0, h = 0;  

    leftA = a.x;
    rightA = a.x + a.w;
    topA = a.y;
    bottomA = a.y + a.h;

    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;

    if (leftB < rightA && rightA < rightB) {
    	h = 1; // right
    }
    if (topB < bottomA && bottomA < bottomB) {
    	v = 4; // bottom
    }
    if (leftB < leftA && leftA < rightB) {
    	h = 3; // left
    }
	if (topB < topA && topA < bottomB) {
    	v = 2; // top
	}

 	if (!v || !h) {
 		return 0; // no collision
 	} else {
 		float hOverlap, vOverlap;
 		if (h == 1)
 			hOverlap = abs(rightA - leftB);
 		if (h == 3)
 			hOverlap = abs(leftA - rightB);
 		if (v == 2)
 			vOverlap = abs(topA - bottomB);
 		if (v == 4)
 			vOverlap = abs(bottomA - topB);
 		// some analysis to determine which collision is more significant, based on which absolute coordinate difference is smaller.

 		return (vOverlap < hOverlap) ? v : h;
 	}
} // This function is not commutative, so think of a as the moving object and b as the object it collides with. The returned int indicates which side of a has collided.

int gamefuncs::collided(Entity e, Entity f, float eContraction, float fContraction)
{
	float epsilon = 17.5;
	//float eta = 7.5;

	SDL_Rect eRect;
	eRect.x = e.getX();
	eRect.y = e.getY();
	eRect.w = e.getWidth() * e.getSize() * eContraction;
	eRect.h = e.getHeight() * e.getSize();

	SDL_Rect fRect;
	fRect.x = f.getX();
	fRect.y = f.getY();
	fRect.w = f.getWidth() * f.getSize() * fContraction;
	fRect.h = f.getHeight() * f.getSize();

	if (e.isPlatform()) {
		eRect.y += epsilon;
		eRect.h -= epsilon;
	}
	if (f.isPlatform()) {
		fRect.y += epsilon;
		fRect.h -= epsilon;
	}

	return collisionDetected(eRect, fRect);
} // Uses collisionDetected() when passed two Entities.

bool gamefuncs::sdlCollided(Entity e, Entity f) 
{
	SDL_Rect eRect;
	eRect.x = e.getX();
	eRect.y = e.getY();
	eRect.w = e.getWidth() * e.getSize();
	eRect.h = e.getHeight() * e.getSize();

	SDL_Rect fRect;
	fRect.x = f.getX();
	fRect.y = f.getY();
	fRect.w = f.getWidth() * f.getSize();
	fRect.h = f.getHeight() * f.getSize();

	return SDL_HasIntersection(&eRect, &fRect);
} // Will return true only if e and f have collided.

bool gamefuncs::touching(Entity playerHere, Entity platformHere)
{
	return (abs(playerHere.getY()+playerHere.getHeight()*playerHere.getSize() - platformHere.getY()) < 10 && !(playerHere.getX() < platformHere.getX()) && !(playerHere.getX() > platformHere.getX()+platformHere.getWidth()*platformHere.getSize())) ? true : false;
} // Detects if the player is on a falling platform by seeing if their distance to it is negligible.

float gamefuncs::distance(float x1, float x2, float y1, float y2) 
{
	return sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
} // Returns the Euclidean Distance between (x1,x2) and (y1,y2).

float gamefuncs::entityDistance(Entity e, Entity f) 
{
	return distance(e.centerOf().first, f.centerOf().first, e.centerOf().second, f.centerOf().second);
}

void gamefuncs::setColour(Entity e, Uint8 r, Uint8 g, Uint8 b)
{
	SDL_SetTextureColorMod(e.getTexture(), r, g, b);
} // Modulates an entity's colour.

void gamefuncs::dopplerEffect(Entity e, Uint8 redshift, Uint8 blueshift) 
{
	setColour(e, 0xFF-blueshift, 0xFF-redshift-blueshift, 0xFF-redshift);
}

void gamefuncs::resetColour(Entity e) 
{
	SDL_SetTextureColorMod(e.getTexture(), 0xFF, 0xFF, 0xFF);
}

Uint8 gamefuncs::getTransparency(Entity e, Uint8* p)
{
	SDL_GetTextureAlphaMod(e.getTexture(), p);
	return *p;
}

void gamefuncs::setTransparency(Entity e, Uint8 a) 
{
	SDL_SetTextureAlphaMod(e.getTexture(), a);
} // Makes an entity more transparent.

void gamefuncs::resetTransparency(Entity e) 
{
	SDL_SetTextureAlphaMod(e.getTexture(), 0xFF);
}

bool gamefuncs::mouseOver(Entity e, int mX, int mY) 
{
	SDL_Rect eRect;
	eRect.x = e.getX();
	eRect.y = e.getY();
	eRect.w = e.getWidth() * e.getSize();
	eRect.h = e.getHeight() * e.getSize();

	SDL_Rect mouse;
	mouse.x = mX;
	mouse.y = mY;
	mouse.w = 2;
	mouse.h = 2;

	return SDL_HasIntersection(&eRect, &mouse);
}

void gamefuncs::playSound(string soundKey, map<string, Mix_Chunk*>& soundMap, int r) 
{
	Mix_PlayChannel(-1, soundMap[soundKey], r);
} // Plays a sound effect and repeats it r times.

void gamefuncs::stopSound()
{
	Mix_HaltChannel(-1);
}

void gamefuncs::startMusic(string musicKey, map<string, Mix_Music*>& musicMap) 
{
	if (!Mix_PlayingMusic())
        Mix_PlayMusic(musicMap[musicKey], -1);
} // Starts the music, if it is not already playing.

void gamefuncs::toggleMusic() 
{
        if (Mix_PausedMusic()) {
            Mix_ResumeMusic();
        } else {
            Mix_PauseMusic();
        }
} // Toggles the given music on or off.

void gamefuncs::stopMusic() 
{
	Mix_HaltMusic();
} // Stops all music.

void gamefuncs::displayText(string text, float delay)
{
	char c;
	float d;
	for (unsigned int i = 0; i < text.length(); i++) {
		c = text.at(i);
		std::cout << c;

		switch(c)
		{
			case '.':
				d = delay * 50;
				break;
			case ',':
				d = delay * 10;
				break;
			default:
				d = delay;
		}

		wait(d);
	}
	std::cout << "\n";
}

