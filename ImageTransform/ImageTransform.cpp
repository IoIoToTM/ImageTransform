// ImageTransform.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include<SDL.h>
#include<SDL_ttf.h>
#include<cmath>
#include<string>
#include<iostream>

SDL_Window* window = NULL;

SDL_Renderer* renderer = NULL;

int getPixel(int x, int y, SDL_Surface* img)
{
	return  x + (y*(img->pitch / sizeof(unsigned int)));
}

SDL_Surface* convertSurfaceWithFactor(SDL_Surface* img, int factor)
{


	SDL_Surface* temp = SDL_DuplicateSurface(img);

	unsigned int* pixels = (unsigned int*)temp->pixels;

	for (int x = 0; x < temp->w; x++)
	{
		for (int y = 0; y < temp->h; y++)
		{
			//Uint32* target = (Uint32*)kitten->pixels + y * kitten->pitch + x * sizeof *target;
			//target = &color;
			int location = getPixel(x, y, temp);

			Uint8 red, green, blue;


			SDL_GetRGB(pixels[location], temp->format, &red, &green, &blue);
			red = std::round(factor*red / 255.0) * (255 / factor);
			green = std::round(factor*green / 255.0) * (255 / factor);
			blue = std::round(factor*blue / 255.0) * (255 / factor);

			//std::cout << "Color at " << x << " " << y << " is " << red << " " << green << " " << blue;

			pixels[location] = SDL_MapRGB(temp->format, red, green, blue);
		}
	}

	return temp;

}

SDL_Surface* ditherSurfaceWithFactor(SDL_Surface* img, int factor)
{


	SDL_Surface* temp = SDL_DuplicateSurface(img);

	Uint32* pixels = (Uint32*)temp->pixels;

	for (int y = 0; y < temp->h; y += 1)
	{
		for (int x = 0; x < temp->w; x += 1)
		{
			//Uint32* target = (Uint32*)kitten->pixels + y * kitten->pitch + x * sizeof *target;
			//target = &color;
			int location = getPixel(x, y, temp);

			Uint8 red, green, blue;

			SDL_GetRGB(pixels[location], temp->format, &red, &green, &blue);
			Uint8 newRed = std::round(factor*red / 255.0) * (255 / factor);
			Uint8 newGreen = std::round(factor*green / 255.0) * (255 / factor);
			Uint8 newBlue = std::round(factor*blue / 255.0) * (255 / factor);

			float errRed = red - newRed;
			float errGreen = green - newGreen;
			float errBlue = blue - newBlue;

			//std::cout << "Color at " << x << " " << y << " is " << red << " " << green << " " << blue;
			//first normal pixel
			pixels[location] = SDL_MapRGB(temp->format, newRed, newGreen, newBlue);



			//other pixels

			location = getPixel(x + 1, y, temp);
			SDL_GetRGB(pixels[location], temp->format, &red, &green, &blue);
			red = red + errRed * 7.0 / 16.0;
			green = green + errGreen * 7.0 / 16.0;
			blue = blue + errBlue * 7.0 / 16.0;
			pixels[location] = SDL_MapRGB(temp->format, red, green, blue);

			location = getPixel(x - 1, y + 1, temp);
			SDL_GetRGB(pixels[location], temp->format, &red, &green, &blue);
			red = red + errRed * 3.0 / 16.0;
			green = green + errGreen * 3.0 / 16.0;
			blue = blue + errBlue * 3.0 / 16.0;
			pixels[location] = SDL_MapRGB(temp->format, red, green, blue);

			location = getPixel(x, y + 1, temp);
			SDL_GetRGB(pixels[location], temp->format, &red, &green, &blue);
			red = red + errRed * 5.0 / 16.0;
			green = green + errGreen * 5.0 / 16.0;
			blue = blue + errBlue * 5.0 / 16.0;
			pixels[location] = SDL_MapRGB(temp->format, red, green, blue);

			location = getPixel(x + 1, y + 1, temp);
			SDL_GetRGB(pixels[location], temp->format, &red, &green, &blue);
			red = red + errRed * 1.0 / 16.0;
			green = green + errGreen * 1.0 / 16.0;
			blue = blue + errBlue * 1.0 / 16.0;
			pixels[location] = SDL_MapRGB(temp->format, red, green, blue);


			//pixels[getPixel(x-1, y+1, img)] = SDL_MapRGB(temp->format, newRed, newGreen, newBlue);
			//pixels[getPixel(x  , y+1, img)] = SDL_MapRGB(temp->format, newRed, newGreen, newBlue);
			//pixels[getPixel(x+1, y+1, img)] = SDL_MapRGB(temp->format, newRed, newGreen, newBlue);
		}
	}

	return temp;

}

void renderTextAt(SDL_Renderer* renderer, const char* text, int x, int y)
{
	TTF_Font* font = TTF_OpenFont("arial.ttf", 32);

	SDL_Color temp = { 255,255,255 };

	SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, temp);
	if (textSurface == NULL)
	{
		std::cout << "Error: " << TTF_GetError();
		return;
	}

	SDL_Texture* tempText = SDL_CreateTextureFromSurface(renderer, textSurface);

	SDL_Rect tempRect = { x,y,textSurface->w,textSurface->h };
	SDL_RenderCopy(renderer, tempText, NULL, &tempRect);

	SDL_FreeSurface(textSurface);
	textSurface = NULL;

	SDL_DestroyTexture(tempText);
	tempText = NULL;

	TTF_CloseFont(font);


}

void blitTextOnSurfaceAt(SDL_Surface* dest, const char* text, int x, int y)
{
	TTF_Font* font = TTF_OpenFont("arial.ttf", 32);
	TTF_Font* fontOutline = TTF_OpenFont("arial.ttf", 32);
	

	TTF_SetFontOutline(fontOutline,1);

	SDL_Color white = { 255,255,255 };
	SDL_Color black = { 0,0,0 };

	//SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, black);

	SDL_Surface *bg_surface = TTF_RenderText_Blended(fontOutline, text, black);
	SDL_Surface *fg_surface = TTF_RenderText_Blended(font, text, white);
	SDL_Rect rect = { 1, 1, fg_surface->w, fg_surface->h };


	SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
	SDL_BlitSurface(fg_surface, NULL, bg_surface, &rect);
	SDL_BlitSurface(bg_surface, NULL, dest, NULL);

	SDL_FreeSurface(fg_surface);
	SDL_FreeSurface(bg_surface);

	/*if (textSurface == NULL)
	{
		std::cout << "Error: " << TTF_GetError();
		return;
	}*/

	//SDL_BlitSurface(textSurface, NULL, dest, NULL);

	//SDL_FreeSurface(textSurface);

	TTF_CloseFont(font);
	TTF_CloseFont(fontOutline);
}


int main(int argc, char* args[])
{

	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	window = SDL_CreateWindow("Images", 100, 100, 700, 700, SDL_WINDOW_SHOWN);

	SDL_SetWindowResizable(window, SDL_TRUE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	SDL_Texture* background = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_TARGET, 700, 700);


	SDL_Surface* loadedImage = NULL;

	bool quit = false;

	SDL_Event e;

	int convertingFactor = 0;

	char* droppedDir;

	SDL_Rect imageRect;

	while (!quit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}

			if (e.type == SDL_KEYUP)
			{
				if (e.key.keysym.sym == SDLK_p)
				{
					convertingFactor++;
					SDL_Surface* temp = convertSurfaceWithFactor(loadedImage, convertingFactor);


					int NumOfColors;

					if (convertingFactor == 0)
					{
						NumOfColors = 256 * 256 * 256;
					}
					else
					{
						NumOfColors = (convertingFactor + 1)*(convertingFactor + 1)*(convertingFactor + 1);
					}
					std::string str = "There are ";

					//renderTextAt(renderer, str.append(std::to_string(NumOfColors)).c_str(), 20, 20);

					blitTextOnSurfaceAt(temp, str.append(std::to_string(NumOfColors).append(" colors")).c_str(), 20, 20);




					SDL_DestroyTexture(background);
					background = SDL_CreateTextureFromSurface(renderer, temp);



					SDL_FreeSurface(temp);

				}
				if (e.key.keysym.sym == SDLK_o)
				{
					convertingFactor--;
					if(convertingFactor <= 0)
					{

						int NumOfColors = 256 * 256 * 256;
						
						convertingFactor = 0;
						SDL_DestroyTexture(background);

						SDL_Surface* temp = SDL_DuplicateSurface(loadedImage);
						std::string str = "There are ";
						blitTextOnSurfaceAt(temp, str.append(std::to_string(NumOfColors).append(" colors")).c_str(), 20, 20);
						background = SDL_CreateTextureFromSurface(renderer, temp);

						SDL_FreeSurface(temp);

						
					}
					else
					{

						int NumOfColors = (convertingFactor + 1)*(convertingFactor + 1)*(convertingFactor + 1);

						SDL_Surface* temp = convertSurfaceWithFactor(loadedImage, convertingFactor);

						std::string str = "There are ";
						blitTextOnSurfaceAt(temp, str.append(std::to_string(NumOfColors).append(" colors")).c_str(), 20, 20);

						SDL_DestroyTexture(background);
						background = SDL_CreateTextureFromSurface(renderer, temp);

						SDL_FreeSurface(temp);
					}
				}
			}
			if (e.type == SDL_DROPFILE)
			{
				droppedDir = e.drop.file;

				//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Dropped a file", droppedDir, window);

				SDL_FreeSurface(loadedImage);

				loadedImage = SDL_LoadBMP(droppedDir);
				loadedImage = SDL_ConvertSurfaceFormat(loadedImage, SDL_GetWindowPixelFormat(window), NULL);
				//loadedImage = convertSurfaceWithFactor(loadedImage, 20);
				

				imageRect = {0,0,loadedImage->w,loadedImage->h};

				SDL_DestroyTexture(background);
				SDL_Surface* temp = SDL_DuplicateSurface(loadedImage);

				

				std::string str = "There are ";
				blitTextOnSurfaceAt(temp, str.append(std::to_string(256*256*256).append(" colors")).c_str(), 20, 20);

				background = SDL_CreateTextureFromSurface(renderer, temp);
				SDL_SetWindowSize(window, loadedImage->w, loadedImage->h);


				
				

			}
			if (e.type == SDL_WINDOWEVENT)
			{
				if (e.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					int w, h;
					SDL_GetWindowSize(window, &w, &h);
					imageRect = { 0,0,w,h };
				}
			}
		}

		SDL_RenderClear(renderer);

		SDL_RenderCopy(renderer, background, NULL, &imageRect);

		


		SDL_RenderPresent(renderer);


	}

    return 0;
}

