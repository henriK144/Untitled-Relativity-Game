#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Entity
{
public:
	Entity(float xCoord, float yCoord, int width, int height, SDL_Texture* tex); 
	float getX(); 
	float getY();
	void setX(float amount); 
	void setY(float amount); 
	void setCoords(float amount1, float amount2);
	void changeX(float amount); 
	void changeY(float amount); 

	int getWidth();
	int getHeight();
	void setWidth(int amount);
	void setHeight(int amount);

	float getSize();
	double getTilt();
	void setSize(float s);
	void setTilt(double degrees);
	void changeSize(float a);
	void changeTilt(double b);

	SDL_Texture* getTexture();
	void setTexture(SDL_Texture* tex);
	void setTextureDebug(SDL_Texture* tex);
	SDL_Rect getFrame();
	void setFrameX(float amount);
	void setFrameY(float amount);

	void hide(); // Makes the entity invisible.
	void show(); // Makes the entity not invisible.
	void toggleVisible(); // Switches whether the entity is visible.
	void vanish(); // Makes the entity both invisible and intangible.
	void unvanish(); // Reverses the above.
	void toggleVanished(); // Switches between the two.
	bool isVanished();

	std::pair<float,float> centerOf(); // Returns the coordinates of the center of the entity.
	int entityCollisionDetected(Entity e); // Detects if the entity has collided with another.
	bool isPlatform();
	void makePlatform(); // Corrects the appearance of collidions with the 3D-styled platforms.
	bool operator==(Entity e); // Defines == on entities according to their source texture.
	bool operator==(SDL_Texture* t); // Can be used as an alternative to the above.
	bool operator!=(Entity e); 
	bool operator!=(SDL_Texture* t); 
	
	virtual void setXPrime(float amount);
	virtual void setYPrime(float amount); // Does nothing, this is just for compatibility with the subclasses.
protected:
	float x, y;
	float size;
	double tilt=0.0; 
	bool vanished;
	bool hitboxAdjust;
	SDL_Rect currentFrame;
	SDL_Texture* texture;
	SDL_Texture* sourceTexture;
}; // An entity is any object in the game. 