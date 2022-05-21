#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <cmath>

#include "Entity.hpp"
#include "Body.hpp"
#include "Surface.hpp"

Body::Body(Entity e, float x_vel, float y_vel, bool grav, bool b, float s) 
: Entity(e.getX(), e.getY(), e.getWidth(), e.getHeight(), e.getTexture())
{

	xPrime = x_vel;
	yPrime = y_vel;
	affectedByGravity = grav;
	bouncy = b;
	size = s;


	xPrimePrime = 0;
	yPrimePrime = 0;

	if (affectedByGravity)
		yPrimePrime = g;
} 

void Body::move(int reverseArg) // This is essentially the game's physics engine, which updates physics-law-adhering objects according to the differential formulas dx/dt = x', dy/dt = y', dx'/dt = x", and dy'/dt = y".
{
	float dt = 0.01; // Equal to the tick rate
	if (reverseArg >= 0)
	{
		xPrime += xPrimePrime * dt;
		yPrime += yPrimePrime * dt;
		x += xPrime * dt; 
		y += yPrime * dt; 
	} else { // if reverseArg = -1, so that move(-1) inverts move().
		y += -1 * yPrime * dt;
		x += -1 * xPrime * dt;
		yPrime -= yPrimePrime * dt;
		xPrime -= xPrimePrime * dt;
	} // move(-1) should be called upon collision with a solid hitbox.

	
}

void Body::stopX() // Stops all movement in the x direction by zeroing velocity and acceleration. Should be called upon hitting a wall.
{
	xPrime = 0;
	xPrimePrime = 0;
}

void Body::stopY() // Stops all movement in the y direction by zeroing velocity and acceleration. Should be called upon hitting the ground.
{
	yPrime = 0;
	yPrimePrime = 0;
}

void Body::stop()
{
	stopX();
	stopY();
}

void Body::jump(int strength)
{
	yPrime = -1 * strength;
	yPrimePrime = g;
} // jump(0) makes the object start falling from rest.

void Body::setXPrime(float amount)
{
	xPrime = amount;
}

void Body::setYPrime(float amount)
{
	yPrime = amount;
}

float Body::getXPrime()
{
	return xPrime;
}

float Body::getYPrime()
{
	return yPrime;
}

float Body::getXPrimePrime()
{
	return xPrimePrime;
}

float Body::getYPrimePrime()
{
	return yPrimePrime;
}

void Body::addVelVector(float direction, float magnitude)
{
	xPrime += magnitude * cos(direction);
	yPrime += magnitude * sin(direction);
}

void Body::addAccelVector(float direction, float magnitude)
{
	xPrimePrime += magnitude * cos(direction);
	yPrimePrime += magnitude * sin(direction);
}

void Body::ifOnEdgeBounce(bool yVelDecay)
{
	float epsilon = currentFrame.w*size; 
	float eta = currentFrame.h*size; 
	if ((x < 0 && xPrime < 0) || (x > 1400-epsilon && xPrime > 0)) {
		xPrime *= -1;
	}
	if ((y < 0 && yPrime < 0) || (y > 750-eta && yPrime > 0)) {
		yPrime *= -1;
		if (yVelDecay) {
			yPrime *= 0.9;
		}
			
	}
}

void Body::ifOnEdgeStop()
{
	int epsilon = currentFrame.w*size; 
	int eta = currentFrame.h*size;  
	if (x < 0 || x > 1400-epsilon) {
		move(-1);
		stopX();
	}
	if (y < 0 || y > 750-eta) {
		move(-1);
		stopY();
	}

}

void Body::bounceX(bool velDecay)
{
	xPrime *= -1;
	if (velDecay)
		xPrime *= 0.9;
}

void Body::bounceY(bool velDecay)
{
	yPrime *= -1;
	if (velDecay)
		yPrime *= 0.9;
}

void Body::bounce(bool velDecay)
{
	bounceX(velDecay);
	bounceY(velDecay);
}

bool Body::isBouncy()
{
	return bouncy;
}

void Body::setBouncy()
{
	bouncy = true;
}