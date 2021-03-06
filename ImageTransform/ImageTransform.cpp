// ImageTransform.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include<SDL.h>
#include<SDL_ttf.h>
#include<cmath>
#include<string>
#include<iostream>
#include<thread>
#include<array>




//global window
SDL_Window* window = NULL;

//global renderer
SDL_Renderer* renderer = NULL;

//global font used by the program so I don't open and close one in methods
TTF_Font* font = NULL;



//get the pixel location at the given coordinates (uses pitch)
int getPixel(int x, int y, SDL_Surface* img)
{
	return  x + (y*(img->pitch / sizeof(unsigned int)));
}


//TODO try SIMD or AVX instructions


//naive slow conversion
SDL_Surface* convertSurfaceWithFactor(SDL_Surface* img, int factor)
{


	//duplicating the surface and getting it's pixels
	SDL_Surface* temp = SDL_DuplicateSurface(img);
	unsigned int* pixels = (unsigned int*)temp->pixels;


	//unsigned 8 bit integers for the values of the colors (0-255)
	Uint8 red, green, blue, alpha;


	//depending on the factor we get so many colors, factor 1 gives either 0 or 255
	//factor 2 - 0,122,255, and etc.
	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;

	//going through the picture by x and y coordinated, poor cache coherence so it's slow
	for (int x = 0; x < temp->w; x++)
	{
		for (int y = 0; y < temp->h; y++)
		{
			//the location of the pixel
			int location = getPixel(x, y, temp);

			

			//getting the colors
			SDL_GetRGBA(pixels[location], temp->format, &red, &green, &blue,&alpha);


			


			Uint32 pixelColor = ((Uint32)red <<24) | ((Uint32)green << 16)| ((Uint32)blue << 8)| alpha;

			//computing pixel color values
			red = std::round(red*toRoundedInt) * toTrueValue;
			green = std::round(green*toRoundedInt) * toTrueValue;
			blue = std::round(blue*toRoundedInt) * toTrueValue;
			

			//setting the pixels
			pixels[location] = SDL_MapRGB(temp->format, red, green, blue);
		}
	}

	//return the changed surface
	return temp;
}

/*
SDL_Surface*  convertOnlyPixelAt(SDL_Surface* img,int pos ,int factor)
{

	SDL_Surface*temp = SDL_DuplicateSurface(img);
	unsigned int* pixels = (unsigned int*)img->pixels;

	//int pixelLoc = getPixel(x, y, img);
	Uint8 red, green, blue, alpha;
	
	for (int i = 0; i < 4; i++)
	{
		SDL_GetRGBA(pixels[pos+i], temp->format, &red, &green, &blue, &alpha);

		double test = 255 / factor;
		double test2 = factor / 255.0;
		red = std::round(red*test2) * test;
		green = std::round(green*test2) * test;
		blue = std::round(blue *test2) * test;

		pixels[pos+i] = SDL_MapRGB(temp->format, red, green, blue);
	}

}*/


//using consecutive memory chuncks so there's less cache misses
void convertSurfaceWithFactorCashCoherence(SDL_Surface* img, int factor)
{

	unsigned int* pixels = (unsigned int*)img->pixels;

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;
	
	
	for (int i = 0; i < nPixels; i++)
	{
		Uint8 red, green, blue, alpha;

		SDL_GetRGBA(pixels[i], img->format, &red, &green, &blue, &alpha);

		red = std::round(red*toRoundedInt) * toTrueValue;
		green = std::round(green*toRoundedInt) * toTrueValue;
		blue = std::round(blue *toRoundedInt) * toTrueValue;

		pixels[i] = SDL_MapRGB(img->format, red, green, blue);

	}

}


//using a pointer as a loop rather than an int
void convertSurfaceWithFactorCashCoherencePointerLoop(SDL_Surface* img, int factor)
{

	unsigned int* pixels = (unsigned int*)img->pixels;

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;


	for (Uint32* i = pixels, *end = &pixels[img->pitch* img->h]; i < end; i++)
	{
		Uint8 red, green, blue, alpha;

		SDL_GetRGBA(*i, img->format, &red, &green, &blue, &alpha);

		red = std::round(red*toRoundedInt) * toTrueValue;
		green = std::round(green*toRoundedInt) * toTrueValue;
		blue = std::round(blue *toRoundedInt) * toTrueValue;

		*i = SDL_MapRGB(img->format, red, green, blue);

	}

}


// testing SSE
void testSSE(SDL_Surface* img,const int factor)
{
	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	unsigned int* pixels = (unsigned int*)img->pixels;

	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;

	__m256i onlyBlueMask = _mm256_set1_epi32(0x000000ff);
	__m256i onlyRedMask = _mm256_set1_epi32(0x00ff0000);
	__m256i onlyGreenMask = _mm256_set1_epi32(0x0000ff00);

	__m256d roundedTest = _mm256_set1_pd(toRoundedInt);
	__m256i toTrueValueTest = _mm256_set1_epi32(toTrueValue);

	for (int i = 0; i < nPixels; i += 8)
	{

		/*__m256i curPixelGroup = _mm256_loadu_si256((__m256i*)(&pixels[i]));
		

		__m256i bluePixels = _mm256_and_si256(curPixelGroup,onlyBlueMask);
		__m256i redPixels = _mm256_and_si256(curPixelGroup, onlyRedMask);
		__m256i greenPixels = _mm256_and_si256(curPixelGroup, onlyGreenMask);

		redPixels = _mm256_mullo_epi32(redPixels, roundedTest);
		greenPixels = _mm256_mullo_epi32(greenPixels, roundedTest);
		bluePixels = _mm256_mullo_epi32(bluePixels, roundedTest);

		redPixels = _mm256_mullo_epi32(redPixels, toTrueValueTest);
		greenPixels = _mm256_mullo_epi32(greenPixels, toTrueValueTest);
		bluePixels = _mm256_mullo_epi32(bluePixels, toTrueValueTest);


		__m256i newPixelGroup = _mm256_or_si256(redPixels,greenPixels);
		newPixelGroup = _mm256_or_si256(newPixelGroup, bluePixels);*/

		//__m256i newGroup = _mm256_shuffle_epi32(curPixelGroup, 7);
		//_mm256_storeu_si256((__m256i*)&pixels[i], newPixelGroup);
	}


}


//using SSE instruction to perform a blue mask on eight pixels at a time (256 bits)
void convertSurfaceWithFactorCashCoherenceSIMD(SDL_Surface* img, int factor)
{

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	unsigned int* pixels = (unsigned int*)img->pixels;

	
	__m256i noBlueMask = _mm256_set1_epi32(0xffffff00);
	for (int i = 0; i < nPixels; i += 8)
	{
		__m256i curPixelGroup = _mm256_loadu_si256((__m256i*)(&pixels[i]));		// This is a group of 8 pixels
		
		__m256i newPixelGroup = _mm256_and_si256(curPixelGroup, noBlueMask);
		_mm256_storeu_si256((__m256i*)&pixels[i], newPixelGroup);		// store with unaligned instruction
	}

	/*
	unsigned int* pixels = (unsigned int*)img->pixels;

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;


	for (int i = 0; i < nPixels; i++)
	{
		Uint8 red, green, blue, alpha;

		SDL_GetRGBA(pixels[i], img->format, &red, &green, &blue, &alpha);

		red = std::round(red*toRoundedInt) * toTrueValue;
		green = std::round(green*toRoundedInt) * toTrueValue;
		blue = std::round(blue *toRoundedInt) * toTrueValue;

		pixels[i] = SDL_MapRGB(img->format, red, green, blue);

	}*/

}



//using the blue mask on each pixel
void convertSurfaceWithFactorCashCoherenceNOSIMD(SDL_Surface* img, int factor)
{

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	unsigned int* pixels = (unsigned int*)img->pixels;
	for (int i = 0; i < nPixels; i += 1)
	{
		pixels[i] = pixels[i] & 0xffff0000;



	}
}


//using openMP to pararellize the pixel manipulation as there is no dependency between each pixel
void convertSurfaceWithFactorCashCoherenceOMP(SDL_Surface* img, int factor)
{

	unsigned int* pixels = (unsigned int*)img->pixels;

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;
	

	//pragma that pararellizes the for loop with a pool of threads
	#pragma omp parallel for 
	for (int i = 0; i < nPixels; i++)
	{
		Uint8 red, green, blue, alpha;
		SDL_GetRGBA(pixels[i], img->format, &red, &green, &blue, &alpha);

		red = std::round(red*toRoundedInt) * toTrueValue;
		green = std::round(green*toRoundedInt) * toTrueValue;
		blue = std::round(blue *toRoundedInt) * toTrueValue;

		pixels[i] = SDL_MapRGB(img->format, red, green, blue);

	}

}


//testing openMP with different scheduling for the loop
void convertSurfaceWithFactorCashCoherenceOMPSCHEDULE(SDL_Surface* img, int factor)
{

	unsigned int* pixels = (unsigned int*)img->pixels;

	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;
	

	#pragma omp parallel for schedule(static)
	for (int i = 0; i < nPixels; i++)
	{

		Uint8 red, green, blue, alpha;
		SDL_GetRGBA(pixels[i], img->format, &red, &green, &blue, &alpha);

		red = std::round(red*toRoundedInt) * toTrueValue;
		green = std::round(green*toRoundedInt) * toTrueValue;
		blue = std::round(blue *toRoundedInt) * toTrueValue;

		pixels[i] = SDL_MapRGB(img->format, red, green, blue);
	}

}



//the same as convertSurfaceWithFactor, excepts it changes the given surface and returns nothing (I did this for multhithreading)
//using the cash coherence version as it's faster
void convertSurfaceWithFactorVoid(SDL_Surface* img, int factor)
{

	//get the image pixels and the number of pixels
	unsigned int* pixels = (unsigned int*)img->pixels;
	int nPixels = (img->h*(img->pitch / sizeof(unsigned int)));

	//precomputing values as they don't change
	double toRoundedInt = factor / 255.0;
	double toTrueValue = 255 / factor;


	//going through each pixel
	for (int i = 0; i < nPixels; i++)
	{

			Uint8 red, green, blue,alpha;


			SDL_GetRGBA(pixels[i], img->format, &red, &green, &blue,&alpha);


			red = std::round(red*toRoundedInt) * toTrueValue;
			green = std::round(green*toRoundedInt) * toTrueValue;
			blue = std::round(blue *toRoundedInt) * toTrueValue;

			pixels[i] = SDL_MapRGBA(img->format, red, green, blue,alpha);
		
	}

}

//using 4 threads to proccess 4 parts of the image at a time
void convertSurfaceWithFactorMultiThreaded(SDL_Surface* img, int factor)
{

	Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	//creating 4 surfaces for the 4 corners
	SDL_Surface* upLeft = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);
	SDL_Surface* upRight = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);
	SDL_Surface* downLeft = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);
	SDL_Surface* downRight = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);


	//splitting the image in the 4 smaller surfaces with the help of a rectangle
	SDL_Rect temp = { 0,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, upLeft, NULL);

	temp = { img->w / 2,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, upRight, NULL);

	temp = { 0,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, downLeft, NULL);

	temp = { img->w / 2,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, downRight, NULL);


	//starting 4 threads for each surface
	std::thread t1(convertSurfaceWithFactorCashCoherence, upLeft, factor);
	std::thread t2(convertSurfaceWithFactorCashCoherence, upRight, factor);
	std::thread t3(convertSurfaceWithFactorCashCoherence, downLeft, factor);
	std::thread t4(convertSurfaceWithFactorCashCoherence, downRight, factor);

	//waiting for them to join
	t1.join();
	t2.join();
	t3.join();
	t4.join();

	/*
	convertSurfaceWithFactorVoid(upLeft, upLeft, factor);
	convertSurfaceWithFactorVoid(upRight, upRight, factor);
	convertSurfaceWithFactorVoid(downLeft, downLeft, factor);
	convertSurfaceWithFactorVoid(downRight, downRight, factor);*/


	//creating a new surface with the normal width and height to put the smaller images on
	//SDL_Surface* tempSurface = SDL_CreateRGBSurface(0, img->w, img->h, 32, rmask, gmask, bmask, amask);

	temp = { 0,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(upLeft, NULL, img, &temp);

	temp = { img->w / 2,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(upRight, NULL, img, &temp);

	temp = { 0,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(downLeft, NULL, img, &temp);

	temp = { img->w / 2,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(downRight, NULL, img, &temp);



	//freeing memory
	SDL_FreeSurface(upLeft);
	SDL_FreeSurface(upRight);
	SDL_FreeSurface(downLeft);
	SDL_FreeSurface(downRight);

	return;

}

#include<vector>



//using x threads to proccess x parts of the image at a time, actually slower in most cases
void convertSurfaceWithFactorMultiThreaded3(SDL_Surface* img, int factor,int n)
{

	Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif


	std::vector <SDL_Surface*> surfaces;

	long long power = (std::pow(2, n));
	long long power2 = std::pow(4, n);


	int imgW = img->w / power;
	int imgH = img->h / power;

	for (long long i = 0; i < power2; i++)
	{
		surfaces.push_back(SDL_CreateRGBSurface(0, imgW, imgH, 32, rmask, gmask, bmask, amask));
	}
	//SDL_Surface* temp1 = SDL_CreateRGBSurface(0, img->w/ power, img->h/ power, 32, rmask, gmask, bmask, amask);

	SDL_Rect temp = { 0,0, imgW, imgH };

	long long counter = 0;

	for (int x = 0; x < img->w; x+=imgW)
	{
		for (int y = 0; y < img->h; y+=imgH)
		{
			SDL_BlitSurface(img,&temp , surfaces.at(counter) , NULL);
			counter++;
			temp.y += imgH;
		}

		temp.x += imgW;
		temp.y = 0;
	}

	std::vector <std::thread> threads;

	for (int j = 0; j < power2; j++)
	{
		threads.push_back(std::thread(convertSurfaceWithFactorCashCoherence, surfaces.at(j), factor));
	}

	for (int j = 0; j < power2; j++)
	{
		threads.at(j).join();
	}

	//starting 4 threads for each surface

	//waiting for them to join


	/*
	convertSurfaceWithFactorVoid(upLeft, upLeft, factor);
	convertSurfaceWithFactorVoid(upRight, upRight, factor);
	convertSurfaceWithFactorVoid(downLeft, downLeft, factor);
	convertSurfaceWithFactorVoid(downRight, downRight, factor);*/


	//creating a new surface with the normal width and height to put the smaller images on
	//SDL_Surface* tempSurface = SDL_CreateRGBSurface(0, img->w, img->h, 32, rmask, gmask, bmask, amask);

	temp = { 0,0, imgW, imgH };

	counter = 0;

	for (int x = 0; x < img->w; x += imgW)
	{
		for (int y = 0; y < img->h; y += imgH)
		{
			SDL_BlitSurface(surfaces.at(counter), NULL, img, &temp);
			counter++;
			temp.y += imgH;
		}

		temp.x += imgW;
		temp.y = 0;
	}




	//freeing memory
	
	for (long long j = 0; j < power2; j++)
	{
		SDL_FreeSurface(surfaces.at(j));
	}
	//return tempSurface;
	return;

}

//using 4*n threads to proccess 4*n parts of the image at a time (recursive using n)
void convertSurfaceWithFactorMultiThreaded1(SDL_Surface* img, int factor,int n)
{

	if (n == 0)
	{
		convertSurfaceWithFactorMultiThreaded(img,factor);
		return;
	}

	Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif



	//creating 4 surfaces for the 4 corners
	SDL_Surface* upLeft = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);
	SDL_Surface* upRight = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);
	SDL_Surface* downLeft = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);
	SDL_Surface* downRight = SDL_CreateRGBSurface(0, img->w / 2, img->h / 2, 32, rmask, gmask, bmask, amask);


	//splitting the image in the 4 smaller surfaces with the help of a rectangle
	SDL_Rect temp = { 0,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, upLeft, NULL);

	temp = { img->w / 2,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, upRight, NULL);

	temp = { 0,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, downLeft, NULL);

	temp = { img->w / 2,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(img, &temp, downRight, NULL);


	n -= 1;

	//starting 4 threads for each surface
	std::thread t1(convertSurfaceWithFactorMultiThreaded1, upLeft, factor,n);
	std::thread t2(convertSurfaceWithFactorMultiThreaded1, upRight, factor,n);
	std::thread t3(convertSurfaceWithFactorMultiThreaded1, downLeft, factor,n);
	std::thread t4(convertSurfaceWithFactorMultiThreaded1, downRight, factor,n);

	//waiting for them to join
	t1.join();
	t2.join();
	t3.join();
	t4.join();

	/*
	convertSurfaceWithFactorVoid(upLeft, upLeft, factor);
	convertSurfaceWithFactorVoid(upRight, upRight, factor);
	convertSurfaceWithFactorVoid(downLeft, downLeft, factor);
	convertSurfaceWithFactorVoid(downRight, downRight, factor);*/


	//creating a new surface with the normal width and height to put the smaller images on
	//SDL_Surface* tempSurface = SDL_CreateRGBSurface(0, img->w, img->h, 32, rmask, gmask, bmask, amask);

	temp = { 0,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(upLeft, NULL, img, &temp);

	temp = { img->w / 2,0,img->w / 2,img->h / 2 };
	SDL_BlitSurface(upRight, NULL, img, &temp);

	temp = { 0,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(downLeft, NULL, img, &temp);

	temp = { img->w / 2,img->h / 2,img->w / 2,img->h / 2 };
	SDL_BlitSurface(downRight, NULL, img, &temp);



	//freeing memory
	SDL_FreeSurface(upLeft);
	SDL_FreeSurface(upRight);
	SDL_FreeSurface(downLeft);
	SDL_FreeSurface(downRight);

	return;

}


//dithering algorithm
SDL_Surface* ditherSurfaceWithFactor(SDL_Surface* img, int factor)
{


	SDL_Surface* temp = SDL_DuplicateSurface(img);

	Uint32* pixels = (Uint32*)temp->pixels;

	//using openMP on the outer loop causes some lines
	for (int y = 0; y < temp->h; y += 1)
	{


////////#pragma omp parallel for
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


//rendering text at the given coordinates
void renderTextAt(SDL_Renderer* renderer, const char* text, int x, int y)
{
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
}

//TODO convert using GPU (textures)
SDL_Texture* convertUsingTextures(SDL_Surface* img, int factor)
{
	return NULL;
}



//draw text with outline on given surface and coordinates 
void blitTextOnSurfaceAt(SDL_Surface* dest, const char* text, int x, int y)
{
	TTF_Font* fontOutline = TTF_OpenFont("arial.ttf", 32);

	TTF_SetFontOutline(fontOutline, 1);

	SDL_Color white = { 255,255,255 };
	SDL_Color black = { 0,0,0 };


	SDL_Surface *bg_surface = TTF_RenderText_Blended(fontOutline, text, black);
	SDL_Surface *fg_surface = TTF_RenderText_Blended(font, text, white);
	SDL_Rect rect = { 1, 1, fg_surface->w, fg_surface->h };


	SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
	SDL_BlitSurface(fg_surface, NULL, bg_surface, &rect);
	SDL_BlitSurface(bg_surface, NULL, dest, NULL);

	SDL_FreeSurface(fg_surface);
	SDL_FreeSurface(bg_surface);

	TTF_CloseFont(fontOutline);
}

bool init();
bool loadMedia();
void quit();


//testing algorithms for average speed
void testAlgorithms(SDL_Surface* img, int numberOfGoes)
{
	clock_t before_time = clock();
	clock_t after_time = before_time;

	double averageTime = 0;


	SDL_Surface* temp = SDL_DuplicateSurface(img);

	for (int i = 0; i < numberOfGoes; i++)
	{
		before_time = clock();
		temp = SDL_DuplicateSurface(img);
		convertSurfaceWithFactorCashCoherence(temp, 1);
		after_time = clock();
		SDL_FreeSurface(temp);

		averageTime += (double)(after_time - before_time);
	}

	averageTime /= numberOfGoes;

	std::cout << "Average number of time for " << numberOfGoes << " number of goes for convertSurfaceWithFactorCashCoherence : " << averageTime << std::endl;


	averageTime = 0;

	for (int i = 0; i < numberOfGoes; i++)
	{
		before_time = clock();
		temp = SDL_DuplicateSurface(img);
		convertSurfaceWithFactorCashCoherencePointerLoop(temp, 1);
		after_time = clock();
		SDL_FreeSurface(temp);

		averageTime += (double)(after_time - before_time);
	}

	averageTime /= numberOfGoes;

	std::cout << "Average number of time for " << numberOfGoes << " number of goes for convertSurfaceWithFactorCashCoherence with pointer loop : " << averageTime << std::endl;


	SDL_FreeSurface(temp);
	temp = NULL;

}


//printing some general information about the renderer you're using and supported pixel formats
void printRenderingInfo()
{
	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);

	std::cout << "Renderer name: " << info.name << std::endl;
	std::cout << "Texture formats: " << std::endl;

	for (Uint32 i = 0; i < info.num_texture_formats; i++)
	{
		std::cout << SDL_GetPixelFormatName(info.texture_formats[i]) << std::endl;
	}
}

int main(int argc, char* args[])
{

	if (init() == false)
	{
		std::cout << "Quitting because of initializing error";
		return -1;
 	}

	if (loadMedia() == false)
	{
		std::cout << "Quitting because of loading error";
		return -2;
	}

	
	
	SDL_Texture* background = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_TARGET,700,700);

	SDL_Surface* loadedImage = NULL;


	bool running = true;

	SDL_Event e;

	int convertingFactor = 0;

	char* droppedDir;

	SDL_Rect imageRect;


	while (running)
	{

		//polling the event queue
		while (SDL_PollEvent(&e) != 0)
		{

			//if it's a QUIT event, then quit
			if (e.type == SDL_QUIT)
			{
				running = false;
			}

			//if it's a keyUP event, check which key
			if (e.type == SDL_KEYUP)
			{


				if (e.key.keysym.sym == SDLK_p)
				{

					convertingFactor++;
					SDL_Surface* temp = SDL_DuplicateSurface(loadedImage);
					convertSurfaceWithFactorCashCoherence(temp, convertingFactor); 


					//getting number of colors in image as it's quantizing to a number of colors
					//TODO create a function that does this 
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

					blitTextOnSurfaceAt(temp, str.append(std::to_string(NumOfColors).append(" colors")).c_str(), 20, 20);

					//destroying the texture and creating a new one
					SDL_DestroyTexture(background);
					background = SDL_CreateTextureFromSurface(renderer, temp);
				
					//freeing the temp surface
					SDL_FreeSurface(temp);
					temp = NULL;
				}
				
				//the same as above - the factor
				if (e.key.keysym.sym == SDLK_o)
				{
					convertingFactor--;
					if (convertingFactor <= 0)
					{
						int NumOfColors = 256 * 256 * 256;

						convertingFactor = 0;
						SDL_DestroyTexture(background);

						SDL_Surface* temp = SDL_DuplicateSurface(loadedImage);
						std::string str = "There are ";
						blitTextOnSurfaceAt(temp, str.append(std::to_string(NumOfColors).append(" colors")).c_str(), 20, 20);


						SDL_DestroyTexture(background);
						background = SDL_CreateTextureFromSurface(renderer, temp);

						SDL_FreeSurface(temp);
						temp = NULL;
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
						temp = NULL;
					}
				}

				//using this to test the algorithms I've put
				if (e.key.keysym.sym == SDLK_w)
				{
					testAlgorithms(loadedImage, 20);
				}

				//testing again
				if (e.key.keysym.sym == SDLK_b)
				{

					convertingFactor++;
				
					testSSE(loadedImage,(const int)convertingFactor);

					
					blitTextOnSurfaceAt(loadedImage, "hello", 1, 1);

					SDL_DestroyTexture(background);

					background = SDL_CreateTextureFromSurface(renderer, loadedImage);

				}
			}

			//if it's a DROPFILE event, check the file and load it (for now only BMP as I'm not using SDL_Image)
			if (e.type == SDL_DROPFILE)
			{
				//get the file path
				droppedDir = e.drop.file;
				//reset the converting factor
				convertingFactor = 0;

				//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Dropped a file", droppedDir, window);

				//free the surface
				SDL_FreeSurface(loadedImage);
				loadedImage = NULL;

				//load the BMP
				loadedImage = SDL_LoadBMP(droppedDir);

				//IMPORTANT convert it to the window format because there are lines if not
				loadedImage = SDL_ConvertSurfaceFormat(loadedImage, SDL_GetWindowPixelFormat(window), NULL);

				//getting the image length and height
				imageRect = { 0,0,loadedImage->w,loadedImage->h };

				SDL_Surface* temp = SDL_DuplicateSurface(loadedImage);

				std::string str = "There are ";
				blitTextOnSurfaceAt(temp, str.append(std::to_string(256 * 256 * 256).append(" colors")).c_str(), 20, 20);

				SDL_DestroyTexture(background);

				background = SDL_CreateTextureFromSurface(renderer, temp);
				SDL_SetWindowSize(window, loadedImage->w, loadedImage->h);

				SDL_FreeSurface(temp);
				temp = NULL;

			}

			//if it's a window event and if it's a resizing one, resize the image rect
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

	quit();

	return 0;
}



bool init()
{

	bool success = true;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		success = false;
		std::cout << "There was an error initializing SDL, error: "<<SDL_GetError()<<std::endl;
	}
	else 
	{
		window = SDL_CreateWindow("Images", 100, 100, 700, 700, SDL_WINDOW_SHOWN);

		if (window == NULL)
		{
			success = false;
			std::cout << "There was an error creating window, error: " << SDL_GetError() << std::endl;
		}
		else
		{
			SDL_SetWindowResizable(window, SDL_TRUE);


			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
			if (renderer == NULL)
			{
				success = false;
				std::cout << "There was an error creating renderer, error: " << SDL_GetError() << std::endl;
			}
			else
			{
				if (TTF_Init() != 0)
				{
					success = false;
					std::cout << "There was an error initializing SDL_TTF, error: " << TTF_GetError() << std::endl;
				}
				
			}
		}

		
	}
	
	return success;

}


bool loadMedia()
{
	bool success = true;

	font = TTF_OpenFont("arial.ttf", 32);

	if (font == NULL)
	{
		success = false;
		std::cout << "There was an error loading arial.ttf, check if file is in directory, error: " << TTF_GetError() << std::endl;
	}

	return success;
}

void quit()
{

	TTF_Quit();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_DestroyWindow(window);


}
