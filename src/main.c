/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Damian Waradzyn
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <memory.h>
#include <curl/curl.h>
#include <malloc.h>
#include <locale.h>

#include <SDL/SDL.h>
//#include <SDL/SDL_ttf.h>

#ifdef N900
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>
LocationGPSDevice *device;
LocationGPSDControl *control;
#endif

#include "main.h"
#include "tile.h"
#include "network.h"
#include "texture.h"
#include "screen.h"
#include "tileProviders.c"

#include "taskman.c"
#include "tileengine.h"
#include "glcanvas.c"
#include "accelerometer.c"
#include "geocoding/geocoder.h"
#include "routing/router.h"
#include "buildings3d.c"
#include "input.c"
#include "config.c"
#include "location.c"
#include "battery.c"
#include "navigation/navigation.h"

OptionsStruct options;

Canvas canvas;

struct {
	double latitude, longitude;
} location;

Accelerometer accelerometer;
Mouse mouse;
SearchBar searchBar;

GQueue* downloadQueue;

int tileCoordinateVisibility[TILES_X + 1][TILES_Y + 1];

int quit = FALSE;
long nowMillis = 0, previousMillis = 0, diffMillis = 0;
int downloaded = 0;
int batteryPercentage;
long lastBatteryRefresh = 0, lastGarbageCollection = 0;

volatile int syncLoadedTilesAppearance = TRUE;
int forceGarbageCollection = FALSE;
int texturesCreatedThisFrame = 0;

int currentTilesIdx;
GLfloat fps;

int SCREEN_WIDTH = 0;
int SCREEN_HEIGHT = 0;

void setQuitFlag() {
	quit = TRUE;
}

//void takeScreenshot() {
//	SDL_Surface * image = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
//	glReadBuffer(GL_FRONT);
//	glReadPixels(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
//	sprintf(strbuf, "pic-%08l.bmp", nowMillis);
//	SDL_SaveBMP(image, strbuf);
//	SDL_FreeSurface(image);
//}

int main(int argc, char **argv) {
	//    SDL_Surface *surface;
	setlocale(LC_CTYPE, NULL);

	SDL_Event event;
//	SDL_TextInputEvent text;

	/* whether or not the window is active */
	int isActive = TRUE;

	initGL();
#if defined(N900) || defined(N950)
	SDL_ShowCursor(0);
#else
	SDL_ShowCursor(1);
#endif


//	if (TTF_Init() == -1) {
//		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
//		exit(2);
//	}

	loadUI();
	loadConfig();
	initInput();

	initTileEngine();
	if (options.loadLastRoute) {
		loadRoute(NULL);
	}
	initLocation();
	initBattery();
	curl_global_init(CURL_GLOBAL_ALL);

	/* wait for events */
	quit = FALSE;
	while (!quit) {
		/* handle the events in the queue */
		previousMillis = nowMillis;
		nowMillis = SDL_GetTicks();
		diffMillis = nowMillis - previousMillis;
		texturesCreatedThisFrame = 0;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_ACTIVEEVENT:
				if (event.active.gain == 0) {
					isActive = FALSE;
					saveConfig();
				} else {
					isActive = TRUE;
				}
				break;
			case SDL_QUIT:
				quit = TRUE;
				break;
			case SDL_KEYDOWN:
				processKeyDown(event.key);
				break;
			case SDL_KEYUP:
				processKeyUp(event.key);
				break;
//			case SDL_TEXTINPUT:
//				text = event.text;
//				processTextInput(text.text);
//				break;
			default:
				break;
			}
		}
		processKeyboard();
		processGeocoding();
		processAccelerometer();
		tileEngineUpdateCoordinates();
		processRouting();
		tileEngineUpdateMarkers();
		setupGlScene();
		processMouse();
		tileEngine_computeBoundingTrapezoid();
		tileEngine_computeTilesVisibility();
		tileEngine_handleZoomTiles();
		updateUi();
		deleteOldConsoleLines();
		updateSyncWaitTiles();
		updateBusyValue();
		updateNavigation();
		// TODO detect screen lock
		if (!isActive) {
			// when our window is inactive save system resources by limiting framerate
#if defined(N900) || defined(N950)
			SDL_Delay(100);
#endif
		}
		drawGLScene();

		// refresh battery percentage and save current view every 10 seconds
		if (nowMillis - lastBatteryRefresh > 10000) {
			refreshBattery();
			lastBatteryRefresh = nowMillis;
			saveConfig();
		}
		if (nowMillis - lastGarbageCollection > 500 || forceGarbageCollection) /*|| g_queue_get_length(allCreatedTiles) > 4 * TILES_X * TILES_Y)*/{
			processDeferredDeallocation();
			lastGarbageCollection = nowMillis;
			forceGarbageCollection = FALSE;
		}
		prevent_screen_dimming();
	}
	saveConfig();
//	TTF_Quit();
	SDL_Quit();
	return 0;
}

/* clean ourselves up and exit */
//    shutdownTileEngine();
//    shutdownLocation();
////    TTF_CloseFont(font);

