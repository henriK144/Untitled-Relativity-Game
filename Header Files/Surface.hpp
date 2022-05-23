#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Surface: public Body
{
public:
	Surface(Entity e, bool rSolid, bool tSolid, bool lSolid, bool bSolid, int dmg=0, float s=1, bool h=false);
	bool isSolid(int i);
	int getDamage();
	void setDamage(int d);
private:
	bool solid[5];
	int damage; 
}; // Surfaces are entities that the player can collide with and/or take damage from.