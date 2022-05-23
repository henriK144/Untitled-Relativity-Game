#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>

#include "Entity.hpp"
#include "Body.hpp"
#include "Surface.hpp"

Surface::Surface(Entity e, bool rSolid, bool tSolid, bool lSolid, bool bSolid, int dmg, float s, bool h)
: Body(e, 0, 0, false, false, s)
{
	solid[0] = false; // Unused.
	solid[1] = rSolid; // true means that the surface's right side is solid.
	solid[2] = tSolid; // true means that the surface's top side is solid.
	solid[3] = lSolid; // true means that the surface's left side is solid.
	solid[4] = bSolid; // true means that the surface's bottom side is solid.

	damage = dmg;
}

bool Surface::isSolid(int i)
{
	return solid[i];
}



int Surface::getDamage()
{
	return damage;
}

void Surface::setDamage(int d)
{
	damage = d;
}
