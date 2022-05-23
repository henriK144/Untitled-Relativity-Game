#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <Windows.h>

#include "RenderWindow.hpp"
#include "Entity.hpp"

RenderWindow::RenderWindow(const char* title, int w, int h)
	:window(nullptr), renderer(nullptr)
{
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);

	if (window == nullptr)
		std::cout << "Window display failed. Error: " << SDL_GetError() << std::endl;

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

SDL_Texture* RenderWindow::loadTexture(const char* filePath) 
{
	SDL_Texture* texture = nullptr;
	texture = IMG_LoadTexture(renderer, filePath);

	if (texture == nullptr)
		std::cout << "Failed to load texture. Error: " << SDL_GetError() << std::endl;

	return texture;
}


void RenderWindow::cleanUp()
{
	SDL_DestroyWindow(window);
}

void RenderWindow::clear()
{
	SDL_RenderClear(renderer);
}

void RenderWindow::render(Entity& e, float scaleFactor, float contractionFactorH, float contractionFactorV, bool flipH, bool flipV, double angle, int centerOffsetX, int centerOffsetY)
{
	SDL_Rect src;
	src.x = 0;
	src.y = 0;
	src.w = e.getWidth();
	src.h = e.getHeight();

	SDL_Rect dst;
	dst.x = e.getX();
	dst.y = e.getY();
	dst.w = src.w * scaleFactor * contractionFactorH;
	dst.h = src.h * scaleFactor * contractionFactorV;

	if (scaleFactor != 1.0) {
		e.setSize(scaleFactor);
	}

	SDL_RendererFlip flipParam;
	const double a = angle;
	SDL_Point p = {static_cast<int>(dst.w / 2) + centerOffsetX, static_cast<int>(dst.h / 2) + centerOffsetY};
	const SDL_Point* centerOfRotation = &p;

	if (flipH && flipV) {
		flipParam = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
	} else if (flipH) {
		flipParam = SDL_FLIP_HORIZONTAL;
	} else if (flipV) {
		flipParam = SDL_FLIP_VERTICAL;
	} else {
		flipParam = SDL_FLIP_NONE;
	}

	if (flipParam == SDL_FLIP_NONE && a == 0) {
		SDL_RenderCopy(renderer, e.getTexture(), &src, &dst);
	} else if (centerOffsetX == 0 && centerOffsetY == 0) {
		SDL_RenderCopyEx(renderer, e.getTexture(), &src, &dst, a, nullptr, flipParam);
	} else {
		SDL_RenderCopyEx(renderer, e.getTexture(), &src, &dst, a, centerOfRotation, flipParam);
	}
	
}

void RenderWindow::renderFullscreen(Entity& e)
{
	SDL_RenderCopy(renderer, e.getTexture(), nullptr, nullptr);
}

void RenderWindow::display()
{
	SDL_RenderPresent(renderer);
}

void RenderWindow::fadeOut(Entity cover, float speed) {
	for (int i = 0; i < 256; i++) {
		SDL_SetTextureAlphaMod(cover.getTexture(), i);
		renderFullscreen(cover);
		display();
		Sleep(static_cast<int>(1000.0 / speed));
		SDL_PumpEvents();
	}
}

void RenderWindow::setFullscreen() 
{
	int s = SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	std::cout << s << '\n';
}

void RenderWindow::exitFullscreen() 
{
	int s = SDL_SetWindowFullscreen(window, 0);
	std::cout << s << '\n';
}