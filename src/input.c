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

#include <SDL/SDL.h>
#include <wchar.h>

#include "main.h"
#include "geocoding/geocoder.h"

void initInput() {
	mouse.x = -1;
	mouse.y = -1;
	mouse.button = -1;
	mouse.oldx = -1;
	mouse.oldy = -1;
	mouse.xdelta = 1;
	mouse.ydelta = 0;
	mouse.oldButton = -1;
	mouse.pressed = -1;
	mouse.moved = -1;
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(250, 30);
}

void processMouse() {

	mouse.oldx = mouse.x;
	mouse.oldy = mouse.y;
	mouse.oldButton = mouse.button;
	mouse.button = SDL_GetMouseState(&mouse.x, &mouse.y);

	if (mouse.button == 0) {
		mouse.xdelta = 0;
		mouse.ydelta = 0;
		mouse.draggedDistanceX = 0;
		mouse.draggedDistanceY = 0;
	}

	if (mouse.button != 0 && mouse.oldButton == 0) {
		mouse.pressed = nowMillis;
		mouse.clickedx = mouse.x;
		mouse.clickedy = mouse.y;
		mouse.draggedDistanceX = 0;
		mouse.draggedDistanceY = 0;
	}

	if (mouse.oldButton == 1 && mouse.button == 1) {
		mouse.xdelta = mouse.x - mouse.oldx;
		mouse.ydelta = mouse.y - mouse.oldy;
		if (mouse.xdelta != 0 || mouse.ydelta != 0) {
			mouse.moved = nowMillis;

			int draggedDistanceX = abs(mouse.x - mouse.clickedx);
			int draggedDistanceY = abs(mouse.y - mouse.clickedy);

			if (draggedDistanceX > mouse.draggedDistanceX) {
				mouse.draggedDistanceX = draggedDistanceX;
			}
			if (draggedDistanceY > mouse.draggedDistanceY) {
				mouse.draggedDistanceY = draggedDistanceY;
			}
		}
	}
	tileEngineProcessMouse(processUiMouse());

}

void computeCursorScreen() {
	int i = 0;
	searchBar.cursorScreen = 0;

	while (i < searchBar.cursorInput) {
		searchBar.cursorScreen += MEDIUM_FONT_METRICS[(unsigned char) searchBar.input[i]];
		i++;
	}
	//	fprintf(stderr, "cursor = %d, screen = %d\n", searchBar.cursorInput, searchBar.cursorScreen);
}

void processTextInput(char* text) {
	if (canvas.searchBarActive != 0) {
		int i = searchBar.cursorInput;
		while (searchBar.input[i] != '\0') {
			i++;
		}

		if (i < SEARCHBAR_MAX_CHARS - 2) {
			while (i > searchBar.cursorInput) {
				searchBar.input[i] = searchBar.input[i - 1];
				i--;
			}
		}
		if (searchBar.cursorInput >= SEARCHBAR_MAX_CHARS - 2) {
			searchBar.cursorInput = SEARCHBAR_MAX_CHARS - 2;
		}

		searchBar.input[searchBar.cursorInput] = text[0];
		if (searchBar.cursorInput <= SEARCHBAR_MAX_CHARS - 2) {
			searchBar.cursorInput++;
		}
		computeCursorScreen();
	}
	//	fprintf(stderr, "textInput: %d %s '%s'\n", strlen(searchBar.input), text, searchBar.input);
}

BackgroundQueryProvider* getNextProvider(GList* allProviders, BackgroundQueryProvider* currentProvider, int reverseSearch) {
	GList* provider = allProviders;

	while (provider != NULL) {
		BackgroundQueryProvider *backgroundQueryProvider = provider -> data;
		if (backgroundQueryProvider == currentProvider) {
			if (reverseSearch) {
				if (provider -> prev != NULL) {
					return provider -> prev -> data;
				} else {
					return g_list_last(allProviders) -> data;
				}
			} else {
				if (provider -> next != NULL) {
					return provider -> next -> data;
				} else {
					return allProviders -> data;
				}
			}
			break;
		}
		provider = provider -> next;
	}
}

void addSearchMarkerAtCanvasCenter();
void processKeyDown(SDL_KeyboardEvent key) {
	freePopupMenu();

	if (canvas.searchBarActive == 0) {
		switch (key.keysym.sym) {
		case SDLK_b:
			if (key.keysym.mod & KMOD_LSHIFT) {
				clearBuildings();
			} else {
				buildings3dtest();
			}
			break;
		case SDLK_g:
			options.showGrid = 1 - options.showGrid;
			break;
		case SDLK_h:
			options.showDebugTiles = 1 - options.showDebugTiles;
			break;
		case SDLK_c:
			options.showCoordinates = 1 - options.showCoordinates;
			break;
		case SDLK_SPACE:
			tileEngineGotomypos();
			break;
		case SDLK_x:
			options.accelerometerEnabled = 1 - options.accelerometerEnabled;
			accelerometer.performCalibration = TRUE;
			break;
		case SDLK_z:
			options.offlineMode = 1 - options.offlineMode;
			if (options.offlineMode) {
				addConsoleLine("Off-line mode activated", 0, 1, 0);
			} else {
				addConsoleLine("Off-line mode deactivated", 0, 1, 0);
			}
			break;
		case SDLK_m:
			options.mipmapToggle = 1 - options.mipmapToggle;
			recreateTiles();
			break;
		case SDLK_p:
			tileEngineChangeOrientation(1 - options.orientation);
			break;
		case SDLK_o:
			addSearchMarkerAtCanvasCenter();
			break;
		case SDLK_u:
			if (key.keysym.mod & KMOD_LSHIFT) {
				tileEngineFreeRoute();
			} else {
				tmpQueryRoute();
			}
			break;
		case SDLK_v:
			addConsoleLine("CloudGPS ver. " CLOUDGPS_VERION, 0, 1, 0);
			break;
		case SDLK_KP_ENTER:
		case SDLK_PAGEUP:
			tileEngineZoomIn();
			break;
		case SDLK_PERIOD:
		case SDLK_PAGEDOWN:
			tileEngineZoomOut();
			break;
		case SDLK_BACKSPACE:
			if (canvas.followingMypos) {
				canvas.attraction = canvas.previousCenter;
				canvas.followingMypos = 0;
				canvas.attractionZooming = 0;
				canvas.attractionToPoint = 1;
			}
			break;
		case SDLK_s: {
			options.geocodingProvider = getNextProvider(geocodingProviders, options.geocodingProvider, key.keysym.mod & KMOD_LSHIFT);
			sprintf(strbuf, "New search provider: %s\n", options.geocodingProvider -> name);
			addConsoleLine(strbuf, 0, 1.0, 0);
		}
			break;
		case SDLK_d: {
			canvas.routeFlybyMode = 1;
		}
			break;
		case SDLK_n: {
			GList *provider = tileProviders;

			while (provider != NULL) {
				TileProvider *tileProvider = provider -> data;
				if (canvas.provider == tileProvider) {
					if (key.keysym.mod & KMOD_LSHIFT) {
						if (provider -> prev != NULL) {
							canvas.provider = provider -> prev -> data;
						} else {
							canvas.provider = g_list_last(tileProviders) -> data;
						}
					} else {
						if (provider -> next != NULL) {
							canvas.provider = provider -> next -> data;
						} else {
							canvas.provider = tileProviders -> data;
						}
					}
					break;
				}
				provider = provider -> next;
			}

			sprintf(strbuf, "New tile provider: %s\n", canvas.provider -> name);
			addConsoleLine(strbuf, 0, 1.0, 0);
			recreateTiles();
		}
			break;
		case SDLK_r: {
			options.routingProvider = getNextProvider(routingProviders, options.routingProvider, key.keysym.mod & KMOD_LSHIFT);
			sprintf(strbuf, "New routing provider: %s\n", options.routingProvider -> name);
			addConsoleLine(strbuf, 0, 1.0, 0);
		}
			break;
		default:
			//			fprintf(stderr, "unhandled key down: key name %s\n", SDL_GetKeyName(key.keysym.sym));
			break;
		}
	} else {
		switch (key.keysym.sym) {
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			processNewSearchQuery(searchBar.input);
			break;
		case SDLK_LEFT:
			if (searchBar.cursorInput > 0) {
				searchBar.cursorInput--;
				computeCursorScreen();
			}
			break;
		case SDLK_RIGHT:
			if (searchBar.cursorInput < strlen(searchBar.input) && searchBar.cursorInput < 99) {
				searchBar.cursorInput++;
				computeCursorScreen();
			}
			break;
		case SDLK_BACKSPACE:
			if (searchBar.cursorInput > 0) {
				searchBar.cursorInput--;
				computeCursorScreen();

				//searchBar.input[searchBar.cursorInput] = '\0';

				int i = searchBar.cursorInput;
				while (searchBar.input[i] != '\0') {
					searchBar.input[i] = searchBar.input[i + 1];
					i++;
				}
			}
			searchBar.lastChange = nowMillis;
			break;
		default:
			if (key.keysym.unicode != 0 && (key.keysym.unicode & 0xFF80) == 0) {
				char buf[2];
				buf[0] = (char) (key.keysym.unicode & 0x7F);
				buf[1] = 0;
				processTextInput(buf);
			}
			break;
		}
	}
}

void processKeyUp(SDL_KeyboardEvent key) {

}

Uint8* keys = NULL;
void processKeyboard() {
	if (keys == NULL) {
		keys = SDL_GetKeyState(NULL);
	}

	if (canvas.searchBarActive == 0) {
		if (keys[SDLK_q]) {
			setQuitFlag();
		}
		if (keys[SDLK_LEFT]) {
			if (canvas.viewMode == VIEW_3D) {
				canvas.attractedToRotZ = 0;
				canvas.drotz += .2;
			} else {
				canvas.routeFlybyMode = 0;
				canvas.followingMypos = 0;
				canvas.attractionToPoint = 0;
				canvas.friction = 0.97;
				canvas.dx += 0.2 * sin((canvas.rotz + 90 + 90 * options.orientation) * M_PI / 180.0);
				canvas.dy += 0.2 * cos((canvas.rotz + 90 + 90 * options.orientation) * M_PI / 180.0);
			}
		}
		if (keys[SDLK_RIGHT]) {
			if (canvas.viewMode == VIEW_3D) {
				canvas.attractedToRotZ = 0;
				canvas.drotz -= .2;
			} else {
				canvas.routeFlybyMode = 0;
				canvas.followingMypos = 0;
				canvas.friction = 0.97;

				canvas.attractionToPoint = 0;
				canvas.dx += 0.2 * sin((canvas.rotz - 90 + 90 * options.orientation) * M_PI / 180.0);
				canvas.dy += 0.2 * cos((canvas.rotz - 90 + 90 * options.orientation) * M_PI / 180.0);
			}
		}
		if (keys[SDLK_UP]) {
			canvas.routeFlybyMode = 0;
			canvas.followingMypos = 0;
			canvas.attractionToPoint = 0;
			canvas.friction = 0.97;

			canvas.dx += 0.2 * sin((canvas.rotz + 90 * options.orientation) * M_PI / 180.0);
			canvas.dy += 0.2 * cos((canvas.rotz + 90 * options.orientation) * M_PI / 180.0);
		}
		if (keys[SDLK_DOWN]) {
			canvas.routeFlybyMode = 0;
			canvas.followingMypos = 0;
			canvas.attractionToPoint = 0;
			canvas.friction = 0.97;
			canvas.dx += 0.2 * sin((canvas.rotz + 180 + 90 * options.orientation) * M_PI / 180.0);
			canvas.dy += 0.2 * cos((canvas.rotz + 180 + 90 * options.orientation) * M_PI / 180.0);
		}
		if (keys[SDLK_LSHIFT]) {
			activateZoomBar();
		}
	}
}

int liqaccel_read(int *ax, int *ay, int *az);

void processAccelerometer() {
	if (accelerometer.performCalibration) {
		ocnt = 0;
	}
	liqaccel_read(&accelerometer.x, &accelerometer.y, &accelerometer.z);
	if (accelerometer.performCalibration) {
		accelerometer.calibrateX = accelerometer.x;
		accelerometer.calibrateY = accelerometer.y;
		accelerometer.calibrateZ = accelerometer.z;
		accelerometer.performCalibration = FALSE;
	}
	accelerometer.x -= accelerometer.calibrateX;
	accelerometer.y -= accelerometer.calibrateY;
	accelerometer.z -= accelerometer.calibrateZ;
	tileEngineProcessAccelerometer();
}
