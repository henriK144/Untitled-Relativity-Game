#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>

#include "Entity.hpp"
#include "Body.hpp"
#include "Surface.hpp"


Entity::Entity(float xCoord, float yCoord, int width, int height, SDL_Texture* tex)
:x(xCoord), y(yCoord), texture(tex), sourceTexture(tex)
{
	currentFrame.x = 0;
	currentFrame.y = 0;
	currentFrame.w = width;
	currentFrame.h = height;
	size = 1;
	vanished = false;
}

float Entity::getX()
{
	return x;
}

float Entity::getY()
{
	return y;
}

void Entity::changeX(float amount)
{
	x += amount;
}

void Entity::changeY(float amount)
{
	y += amount;
}

void Entity::setX(float amount)
{
	x = amount;
}

void Entity::setY(float amount)
{
	y = amount;
}

void Entity::setCoords(float amount1, float amount2)
{
	x = amount1;
	y = amount2;
}

void Entity::setFrameX(float amount)
{
	currentFrame.x = amount;
}

void Entity::setFrameY(float amount)
{
	currentFrame.y = amount;
}

int Entity::getWidth()
{
	return currentFrame.w;
}

int Entity::getHeight()
{
	return currentFrame.h;
}

SDL_Rect Entity::getFrame()
{
	return currentFrame;
}

void Entity::setWidth(int amount)
{
	currentFrame.w = amount;
}

void Entity::setHeight(int amount)
{
	currentFrame.h = amount;
}

std::pair<float,float> Entity::centerOf()
{
	float actualWidth = currentFrame.w * size;
	float actualHeight = currentFrame.h * size;
	std::pair<float,float> coords;
	coords.first = x + 0.5 * actualWidth;
	coords.second = y + 0.5 * actualHeight;
	return coords;
}

float Entity::getSize()
{
	return size;
}

void Entity::setSize(float s)
{
	size = s;
}

SDL_Texture* Entity::getTexture()
{
	return texture;
}

void Entity::setTexture(SDL_Texture* tex)
{
	texture = tex;
}

void Entity::setTextureDebug(SDL_Texture* tex)
{
	texture = tex;
	std::cout << "called" << '\n';
}

void Entity::setTilt(double degrees)
{
	tilt = degrees;
}

double Entity::getTilt()
{
	return tilt;
}

void Entity::changeSize(float a)
{
	size += a;
}

void Entity::changeTilt(double b)
{
	tilt += b;
}

void Entity::hide()
{	
	sourceTexture = texture;
	texture = nullptr;
}

void Entity::show()
{
	texture = sourceTexture;
}

void Entity::toggleVisible()
{
	if (texture == nullptr) {
		texture = sourceTexture;
	} else {
		sourceTexture = texture;
		texture = nullptr;
	}
}

void Entity::vanish()
{
	if (!vanished) {
		changeY(9999);
		vanished = true;
	}
}

void Entity::unvanish()
{
	if(vanished) {
		changeY(-9999);
		vanished = false;
	}
}

void Entity::toggleVanished()
{
	if(vanished) {
		changeY(-9999);
		vanished = false;
	} else {
		changeY(9999);
		vanished = true;
	}
}

bool Entity::isVanished()
{
	return vanished;
}

bool Entity::isPlatform()
{
	return hitboxAdjust;
}

void Entity::makePlatform()
{
	hitboxAdjust = true;
}

void Entity::setXPrime(float amount)
{
	;
}

void Entity::setYPrime(float amount)
{
	;
}

bool Entity::operator==(Entity e)
{
	return(sourceTexture == e.getTexture() || texture == e.getTexture());
}

bool Entity::operator==(SDL_Texture* t)
{
	return(texture == t);
}

bool Entity::operator!=(Entity e)
{
	return !(*this == e);
}

bool Entity::operator!=(SDL_Texture* t)
{
	return !(*this == t);
}