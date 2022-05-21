#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Body: public Entity
{
public:
	Body(Entity e, float x_vel, float y_vel, bool grav=false, bool b=false, float s=1);
	void move(int reverseArg = 0);
	void jump(int strength);
	
	bool isBouncy();
	void setBouncy();
	void bounceX(bool velDecay=false);
	void bounceY(bool velDecay=false);
	void bounce(bool velDecay=false);
	void stopX();
	void stopY();
	void stop();
	void ifOnEdgeBounce(bool yVelDecay=false); // Bounce reverses all momentum.
	void ifOnEdgeStop(); // Stop cancels all velocity and acceleration.

	float getXPrime();
	float getYPrime();
	float getXPrimePrime();
	float getYPrimePrime();
	void setXPrime(float amount);
	void setYPrime(float amount);
	void addVelVector(float direction, float magnitude); // Adds velocity.
	void addAccelVector(float direction, float magnitude); // Adds acceleration. Direction is in radians, where 0=East. Suitable direction constants are defined in main.cpp.
private:
	float xPrime, yPrime; // Velocity is in pixels per tick.
	float xPrimePrime, yPrimePrime; // Acceleration is in pixels per tick squared.
	bool affectedByGravity;
	bool bouncy;
	static constexpr float g = 9.80665;
}; // Bodies are entities that move according to the principles of kinematics.