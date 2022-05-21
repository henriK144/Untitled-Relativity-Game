#pragma once // So that this isn't copied twice erroneously.
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "Entity.hpp"

class RenderWindow
{
public:
	RenderWindow(const char* title, int w, int h); // Constructor.
	SDL_Texture* loadTexture (const char* filePath); // Loads a texture (sprite) to be displayed.
	void cleanUp(); // Deletes everything to prevent memory leaks.
	void clear(); // Clears the screen before rendering new images.
	void render(Entity& e, float scaleFactor=1.0, float contractionFactorH=1.0, float contractionFactorV=1.0, bool flipH=false, bool flipV=false, double angle=0.0, int centerOffsetX=0, int centerOffsetY=0); // Renders an image.
	void renderFullscreen(Entity& e); // Renders an image in fullscreen.
	void setFullscreen();
	void exitFullscreen();
	void display(); // Displays a rendered image.
	void fadeOut(Entity cover, float speed); // Fades to black or white. The time between frames is 1/speed.
	void fadeIn(Entity cover, float speed); // Fades from black or white. The time between frames is 1/speed. (Not yet functional)
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
}; // The window that the game is displayed from.