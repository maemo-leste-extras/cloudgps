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

#include "main.h"
#include "console.h"
#include "tileengine.h"

extern SearchBar searchBar;
extern Mouse mouse;
extern long nowMillis;
extern int downloaded;
extern int batteryPercentage;
extern GLfloat fps;

GList* uiElems = NULL;

static UiElement* pressedUiElem = NULL;

UiElement *crosshair, *zoomIn, *zoomOut, *position, *gotomypos, *view2d, *view3d,
          *compass, *search, *zoomKnot, *closeApp, *tasks, *busy;
extern Texture searchMarkTexture, searchMarkMask, road;

// those need updating on portrait/landscape change.
GList* volatileUiElems = NULL;

void loadTextureAndMask(char* filename, Texture* texture, Texture* mask) {
	SDL_Surface* textureSurface = NULL;
	SDL_Surface* maskSurface = NULL;
	//    fprintf(stderr, "loading texture from %s\n", filename);
	textureSurface = loadTextureFromFile(filename, NULL, mask != NULL ? &maskSurface : NULL);
	//    fprintf(stderr, "loadTextureFromFile returned: %p\n", textureRGB);
	if (textureSurface != NULL) {
		texture -> size = textureSurface -> w;
		texture -> name = createTexture(textureSurface -> pixels, texture->size, texture-> size, FALSE);

		if (mask != NULL) {
			mask -> size = maskSurface -> w;
			mask -> name = createTexture(maskSurface -> pixels, mask->size, mask-> size, FALSE);
			SDL_FreeSurface(maskSurface);
		}

		SDL_FreeSurface(textureSurface);
	} else {
		fprintf(stderr, "loadTexture %s failed\n", filename);
		exit(-1);
	}
}

void loadTexture(char* filename, Texture* texture) {
	loadTextureAndMask(filename, texture, NULL);
}

UiElement* createUiElement(char* texture, int landscapex, int landscapey, int portraitx, int portraity, void(*handlePressed)(), void(*handleDragged)(), int addToList) {
	UiElement* element = calloc(1, sizeof(UiElement));
	if (element == NULL) {
		fprintf(stderr, "memory allocation failed in createUiElement\n");
		return NULL;
	}

	loadTextureAndMask(texture, &(element -> texture), &(element -> mask));
	//loadTexture(&(element -> mask), mask);

	landscapex -= (element -> texture.size) / 2;
	landscapey -= (element -> texture.size) / 2;
	portraitx -= (element -> texture.size) / 2;
	portraity -= (element -> texture.size) / 2;

	element -> landscapex = landscapex;
	element -> landscapey = landscapey;
	element -> x = landscapex;
	element -> y = landscapey;
	element -> portraitx = portraitx;
	element -> portraity = portraity;
	element -> status = UI_SHOWN;
	element -> color.r = 1.0;
	element -> color.g = 1.0;
	element -> color.b = 1.0;
	element -> color.a = 1.0;
	element -> handlePressed = handlePressed;
	element -> handleDragged = handleDragged;
	element -> texCoords = texCoordsLandscape[0];

	//    fprintf(stderr, "addtoqueue = %d\n", addToQueue);
	if (addToList) {
		uiElems = g_list_append(uiElems, element);
	}

	return element;
}

void destroyUiElement(UiElement* elem) {
	fprintf(stderr, "STUB - destroyUiElement(UiElement* elem)\n");
}

void rotate2d(GLfloat* x, GLfloat* y, double rotation);
void prepareQuadStripForUiElement(UiElement* elem) {
	// Dirty hack to make position always the same size on map.
	if (elem == position) {
		setQuadStripSize(elem->mask.size / canvas.scale, elem->mask.size / canvas.scale, quadStripVertices);
	} else if (elem == busy) {
		setQuadStripSize(elem->mask.size, elem->mask.size, quadStripVertices);
		int i;
		for (i = 0; i < 4; i++) {
			quadStripVertices[i * 3] -= elem->mask.size / 2;
			quadStripVertices[i * 3 + 1] -= elem->mask.size / 2;
			rotate2d(&quadStripVertices[i * 3], &quadStripVertices[i * 3 + 1], nowMillis / 3.0);
		}
	} else {
		setQuadStripSize(elem->mask.size, elem->mask.size, quadStripVertices);
	}
}

void afterUiElementDrawn(UiElement* elem) {
	if (elem == busy) {
		resetQuadStripVertices();
	}
}

void drawUiElement(UiElement* elem) {

	glTranslatef(elem -> x, elem -> y, 0);
	glEnable(GL_BLEND);
	// mask
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	int glerror;
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 6 %d", glerror);
	}
	glColor4f(1.0, 1.0, 1.0, elem->color.a);
	glBindTexture(GL_TEXTURE_2D, elem->mask.name);

	prepareQuadStripForUiElement(elem);

	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, elem->texCoords);

	if (elem != zoomKnot) {
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// texture

	// Dirty hack to make the zoom knot transculent
	if (elem == zoomKnot) {
		glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		int glerror;
		glerror = glGetError();
		if (glerror != GL_NO_ERROR) {
			fprintf(stderr, "error 7 %d", glerror);
		}
	} else {
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		int glerror;
		glerror = glGetError();
		if (glerror != GL_NO_ERROR) {
			fprintf(stderr, "error 8 %d", glerror);
		}
	}
	glColor4f(elem->color.r, elem->color.g, elem->color.b, elem->color.a);
	glBindTexture(GL_TEXTURE_2D, elem-> texture.name);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glTranslatef(-elem -> x, -elem -> y, 0);

	afterUiElementDrawn(elem);
}
void setQuitFlag();

void loadUI() {

glPixelStorei(GL_UNPACK_ALIGNMENT, 2);

	loadTextureAndMask("/usr/share/cloudgps/res/font-medium.png", &fontMedium, &fontMediumMask);

	zoomOut = createUiElement("/usr/share/cloudgps/res/zoom-minus.png", 50, SCREEN_HEIGHT - 150, SCREEN_WIDTH - 150, SCREEN_HEIGHT - 50, &tileEngineZoomOut, &tileEngineZoomOut, TRUE);

	zoomIn = createUiElement("/usr/share/cloudgps/res/zoom-plus.png", SCREEN_WIDTH - 50, SCREEN_HEIGHT - 150, SCREEN_WIDTH - 150, 50, &tileEngineZoomIn, &tileEngineZoomIn, TRUE);
	gotomypos = createUiElement("/usr/share/cloudgps/res/gotomypos.png", SCREEN_WIDTH - 50, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 50, 50, &tileEngineGotomypos, &tileEngineGotomypos, TRUE);
	position = createUiElement("/usr/share/cloudgps/res/position.png", 0, 0, 0, 0, NULL, NULL, TRUE);
	view2d = createUiElement("/usr/share/cloudgps/res/2d.png", 50, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 50, &tileEngineViewMode2D, NULL, TRUE);
	view2d -> status = UI_HIDDEN;
	view3d = createUiElement("/usr/share/cloudgps/res/3d.png", 50, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 50, &tileEngineViewMode3D, NULL, TRUE);
	crosshair = createUiElement("/usr/share/cloudgps/res/crosshair.png", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, NULL, NULL, TRUE);
	compass = createUiElement("/usr/share/cloudgps/res/compass.png", 150, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 50, SCREEN_HEIGHT - 150, &tileEngineToggle2dRotation, NULL, TRUE);
	search = createUiElement("/usr/share/cloudgps/res/search.png", SCREEN_WIDTH - 150, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 50, 150, &tileEngineToggleSearchBar, NULL, TRUE);
	zoomKnot = createUiElement("/usr/share/cloudgps/res/zoom-knot.png", SCREEN_WIDTH - 150, SCREEN_HEIGHT - 50, SCREEN_WIDTH - 50, 150, &tileEngineZoomKnotPressed, &tileEngineZoomKnotDragged, TRUE);
	zoomKnot -> status = UI_HIDDEN;

	tasks = createUiElement("/usr/share/cloudgps/res/tasks.png", 16, 0, 0, 0, &invokeTaskManager, NULL, TRUE);
	tasks -> status = UI_HIDDEN;

	closeApp = createUiElement("/usr/share/cloudgps/res/close.png", SCREEN_WIDTH - 64, 32, 32, 32, &setQuitFlag, NULL, TRUE);
	closeApp -> status = UI_HIDDEN;

	busy = createUiElement("/usr/share/cloudgps/res/busy.png", SCREEN_WIDTH - 96, 32, 32, 32, NULL, NULL, TRUE);
	busy -> status = UI_HIDDEN;

	loadTextureAndMask("/usr/share/cloudgps/res/searchmark.png", &searchMarkTexture, &searchMarkMask);
	loadTextureAndMask("/usr/share/cloudgps/res/route_start.png", &routeStartTexture, &routeStartMask);
	loadTextureAndMask("/usr/share/cloudgps/res/route_end.png", &routeEndTexture, &routeEndMask);
	loadTexture("/usr/share/cloudgps/res/shadow.png", &shadow);
	loadTexture("/usr/share/cloudgps/res/drag_crosshair.png", &dragCrosshair);
	loadTexture("/usr/share/cloudgps/res/road.png", &road);

	volatileUiElems = g_list_append(volatileUiElems, zoomOut);
	volatileUiElems = g_list_append(volatileUiElems, view2d);
	volatileUiElems = g_list_append(volatileUiElems, view3d);
	volatileUiElems = g_list_append(volatileUiElems, compass);
	volatileUiElems = g_list_append(volatileUiElems, search);
	volatileUiElems = g_list_append(volatileUiElems, gotomypos);
	volatileUiElems = g_list_append(volatileUiElems, zoomIn);
}

SDL_Surface * texttmpsurface = NULL;

void drawUiElems() {
	GList* elem = uiElems;

	while (elem != NULL) {
		UiElement* uiElem = (UiElement*) elem -> data;
		if (uiElem -> status != UI_HIDDEN && uiElem != position) {
			drawUiElement(elem -> data);
		}
		elem = elem -> next;
	}
	glTranslatef(compass -> x + compass -> texture.size / 2, compass -> y + compass -> texture.size / 2, 0.0);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	int glerror;
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 9 %d", glerror);
	}
	if (canvas.viewMode == VIEW_2D && canvas.rotation2d == FALSE) {
		glColor4f(.2, .2, .2, 0.6);
	} else {
		glColor4f(.3, .3, 1.0, 0.6);
	}
	glVertexPointer(3, GL_FLOAT, 0, arrowVertices);
	glRotatef(canvas.rotz, 0.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glRotatef(canvas.rotz, 0.0, 0.0, -1.0);

	glTranslatef(-compass -> x - compass -> texture.size / 2, -compass -> y - compass -> texture.size / 2, 0.0);
	glEnable(GL_TEXTURE_2D);

	//
	//    SDL_Color color = { 0, 0, 0 };
	//    SDL_Surface *surface;
	//    if (!(surface = TTF_RenderText_Blended(font, "Hello World!", color))) {
	//        fprintf(stderr, "TTF_RenderText_Shaded failed: %s\n", TTF_GetError());
	//    } else {
	//
	//        if (texttmpsurface == NULL) {
	//            texttmpsurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 512, 512, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	//            if (texttmpsurface == NULL) {
	//                fprintf(stderr, "createRGBsurface texttmpsurface failed \n");
	//                exit(1);
	//            }
	//        }
	//        SDL_Rect rect;
	//        rect.x = 0;
	//        rect.y = 0;
	//        rect.w = surface->w;
	//        rect.h = surface->h;
	//        SDL_BlitSurface(surface, &rect, texttmpsurface, &rect);
	//        SDL_FreeSurface(surface);
	//
	//
	//        GLushort * aaa = convert8888to4444(texttmpsurface, FALSE);
	//        int cropRect[] = {0,0,100, 100};
	//        GLuint texture = 0;
	//        glEnable(GL_TEXTURE_2D);
	//        glGenTextures(1, &texture);
	//        glBindTexture(GL_TEXTURE_2D, texture);
	//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, aaa);
	//        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, cropRect);//crash
	//        m_exgldrawte(0, 0, 1, 256, 256);//crash
	//
	//
	//        createTexture(aaa, surface -> w, surface -> h, FALSE);
	//
	//        //SDL_BlitSurface(text_surface,NULL,screen,NULL);
	//        //perhaps we can reuse it, but I assume not for simplicity.
	//
	//        SDL_FreeSurface(surface);
	//    }
}

void drawZoomBar() {
	int idx = 0, i;

	if (canvas.zoomBarAlpha == 0.0) {
		return;
	}
	if (canvas.orientationTransitionLinear < 80) {
		glTranslatef(0, SCREEN_HEIGHT - canvas.searchBarY, 0);
	} else {
		glTranslatef(SCREEN_WIDTH - canvas.searchBarY, SCREEN_HEIGHT, 0);
		glRotatef(-90, 0.0, 0.0, 1.0);
	}
	if (canvas.orientationTransitionLinear == 80) {
		activateZoomBar();
	}

	GLfloat screenSize = options.orientation == LANDSCAPE ? SCREEN_WIDTH : SCREEN_HEIGHT;
	int zoomLevelCount = canvas.provider -> maxZoom - canvas.provider -> minZoom + 1;

	if (isZoomBarInitialized() == FALSE) {
		for (i = 0; i <= zoomLevelCount; i++) {
			zoomBarLines[idx++] = i * (screenSize - 200) / zoomLevelCount + 100;
			zoomBarLines[idx++] = -148;
			zoomBarLines[idx++] = 0.0;

			zoomBarLines[idx++] = i * (screenSize - 200) / zoomLevelCount + 100;
			zoomBarLines[idx++] = -158;
			zoomBarLines[idx++] = 0.0;

		}
		setZoomBarInitialized(TRUE);
	}

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	int glerror;
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 10 %d", glerror);
	}
	glVertexPointer(3, GL_FLOAT, 0, zoomBarLines);
	glColor4f(1.0 - canvas.zoomBarAlpha, 1.0 - canvas.zoomBarAlpha, 1.0 - canvas.zoomBarAlpha, 1.0);
	glDrawArrays(GL_LINES, 0, zoomLevelCount * 2 + 2);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 11 %d", glerror);
	}
	glColor4f(.4, .4, .4, canvas.zoomBarAlpha);
	glDrawArrays(GL_LINES, 0, zoomLevelCount * 2 + 2 + 8);
	glEnable(GL_TEXTURE_2D);

	GLfloat x = canvas.orientationTransitionLinear < 80 ? zoomKnot -> x : SCREEN_HEIGHT - zoomKnot -> y - 64;
	glTranslatef(x, -180, 0);
	sprintf(strbuf, "zoom = %d", getKnotDestZoom());
	drawStringAlpha(strbuf, canvas.zoomBarAlpha, canvas.zoomBarAlpha, canvas.zoomBarAlpha, 1.0, FALSE, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glTranslatef(-x, 180, 0);

	if (canvas.orientationTransitionLinear < 80) {
		glTranslatef(0, -SCREEN_HEIGHT + canvas.searchBarY, 0);
	} else {
		glRotatef(90, 0.0, 0.0, 1.0);
		glTranslatef(-SCREEN_WIDTH + canvas.searchBarY, -SCREEN_HEIGHT, 0);
	}
}

void drawSearchBar() {
	if (canvas.searchBarY > 0.0) {
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		if (canvas.orientationTransitionLinear < 80) {
			glTranslatef(0, SCREEN_HEIGHT - canvas.searchBarY, 0);
		} else {
			glTranslatef(SCREEN_WIDTH - canvas.searchBarY, SCREEN_HEIGHT, 0);
			glRotatef(-90, 0.0, 0.0, 1.0);
		}

		setQuadStripSize(canvas.orientationTransitionLinear < 80 ? SCREEN_WIDTH : SCREEN_HEIGHT, 64, quadStripVertices);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		int glerror;
		glerror = glGetError();
		if (glerror != GL_NO_ERROR) {
			fprintf(stderr, "error 12 %d", glerror);
		}
		glColor4f(.7, .7, .7, 0.5);
		glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glTranslatef(5, 5, 0);
		setQuadStripSize(canvas.orientationTransitionLinear < 80 ? SCREEN_WIDTH - 10 : SCREEN_HEIGHT - 10, 64 - 10, quadStripVertices);
		glColor4f(0.7, .7, .7, 0.5);
		glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glTranslatef(5, 25, 0);

		if (((nowMillis - searchBar.lastChange) / 200) % 2 == 0) {
			GLfloat cursor[] = { searchBar.cursorScreen, -10, 0, searchBar.cursorScreen, 24, 0 };
			glDisable(GL_BLEND);
			glColor4f(1.0, 1.0, 1.0, 1.0);
			glVertexPointer(3, GL_FLOAT, 0, cursor);
			glDrawArrays(GL_LINES, 0, 2);
		}
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		drawString(searchBar.input, 1.0, 1.0, 1.0, FALSE);
		if (canvas.orientationTransitionLinear < 80) {
			glTranslatef(-10, -SCREEN_HEIGHT + canvas.searchBarY - 30, 0);
		} else {
			glTranslatef(-10, -30, 0);
			glRotatef(90, 0.0, 0.0, 1.0);
			glTranslatef(-SCREEN_WIDTH + canvas.searchBarY, -SCREEN_HEIGHT, 0);
		}
	}
}

GLfloat satelliteBars[18 * MAX_SATTELITES];
GLfloat satelliteBarOutlines[24 * MAX_SATTELITES];
GLfloat satelliteBarColors[4 * 6 * MAX_SATTELITES];
GLfloat satelliteBarOutlineColors[4 * 8 * MAX_SATTELITES];
int satelliteBarOutlinesInitialized = FALSE;

void drawSatelliteBars(GLfloat barHeight, GLfloat barWidth, GLfloat separation) {
	int i;
	GLfloat x;
	if (!satelliteBarOutlinesInitialized) {
		for (i = 0; i < MAX_SATTELITES; i++) {
			x = i * (barWidth + separation);
			/*    1
			 *  -----
			 * |     |
			 * |4    |2
			 * |     |
			 *  -----
			 *    3
			 */
			// 1
			satelliteBarOutlines[i * 24] = x;
			satelliteBarOutlines[i * 24 + 1] = 0.0;
			satelliteBarOutlines[i * 24 + 2] = 0.0;

			satelliteBarOutlines[i * 24 + 3] = x + barWidth;
			satelliteBarOutlines[i * 24 + 4] = 0.0;
			satelliteBarOutlines[i * 24 + 5] = 0.0;

			//2
			satelliteBarOutlines[i * 24 + 6] = x + barWidth;
			satelliteBarOutlines[i * 24 + 7] = 0.0;
			satelliteBarOutlines[i * 24 + 8] = 0.0;

			satelliteBarOutlines[i * 24 + 9] = x + barWidth;
			satelliteBarOutlines[i * 24 + 10] = barHeight;
			satelliteBarOutlines[i * 24 + 11] = 0.0;

			//3
			satelliteBarOutlines[i * 24 + 12] = x + barWidth;
			satelliteBarOutlines[i * 24 + 13] = barHeight;
			satelliteBarOutlines[i * 24 + 14] = 0.0;

			satelliteBarOutlines[i * 24 + 15] = x;
			satelliteBarOutlines[i * 24 + 16] = barHeight;
			satelliteBarOutlines[i * 24 + 17] = 0.0;

			// 4
			satelliteBarOutlines[i * 24 + 18] = x;
			satelliteBarOutlines[i * 24 + 19] = barHeight;
			satelliteBarOutlines[i * 24 + 20] = 0.0;

			satelliteBarOutlines[i * 24 + 21] = x;
			satelliteBarOutlines[i * 24 + 22] = 0.0;
			satelliteBarOutlines[i * 24 + 23] = 0.0;
		}
		satelliteBarOutlinesInitialized = TRUE;
	}

#ifdef N900
	if (device && device -> satellites) {
		int j;
		for (i = 0; i < MAX_SATTELITES; i++) {
			x = i * (barWidth + separation);
			if (i < device -> satellites -> len) {
				LocationGPSDeviceSatellite* sat = device -> satellites -> pdata[i];
				int signalHeight = (sat -> signal_strength) / 2;

				if (sat -> in_use) {
					for (j = 0; j < 8; j++) {
						satelliteBarOutlineColors[i * 32 + j * 4] = 1.0;
						satelliteBarOutlineColors[i * 32 + j * 4 + 1] = 1.0;
						satelliteBarOutlineColors[i * 32 + j * 4 + 2] = 1.0;
						satelliteBarOutlineColors[i * 32 + j * 4 + 3] = 0.5;
						if (j < 6) {
							satelliteBarColors[i * 24 + j * 4] = 1.0;
							satelliteBarColors[i * 24 + j * 4 + 1] = 1.0;
							satelliteBarColors[i * 24 + j * 4 + 2] = 1.0;
							satelliteBarColors[i * 24 + j * 4 + 3] = 0.5;
						}
					}
				} else {
					for (j = 0; j < 8; j++) {
						satelliteBarOutlineColors[i * 32 + j * 4] = 0.4;
						satelliteBarOutlineColors[i * 32 + j * 4 + 1] = 0.4;
						satelliteBarOutlineColors[i * 32 + j * 4 + 2] = 0.4;
						satelliteBarOutlineColors[i * 32 + j * 4 + 3] = 0.5;
						if (j < 6) {
							satelliteBarColors[i * 24 + j * 4] = 0.4;
							satelliteBarColors[i * 24 + j * 4 + 1] = 0.4;
							satelliteBarColors[i * 24 + j * 4 + 2] = 0.4;
							satelliteBarColors[i * 24 + j * 4 + 3] = 0.5;
						}
					}
				}

				/*  1,6         2
				 *    ---------
				 *   | \      |
				 *   |  \     |
				 *   |   \    |
				 *   |    \   |
				 *   |     \  |
				 *   |      \ |
				 *  5 --------3,4
				 *
				 */
				// 1
				satelliteBars[i * 18] = x;
				satelliteBars[i * 18 + 1] = barHeight - 1 - signalHeight;
				satelliteBars[i * 18 + 2] = 0.0;

				// 2
				satelliteBars[i * 18 + 3] = x + barWidth;
				satelliteBars[i * 18 + 4] = barHeight - 1 - signalHeight;
				satelliteBars[i * 18 + 5] = 0.0;

				// 3
				satelliteBars[i * 18 + 6] = x + barWidth;
				satelliteBars[i * 18 + 7] = barHeight - 1;
				satelliteBars[i * 18 + 8] = 0.0;

				// 4
				satelliteBars[i * 18 + 9] = x + barWidth;
				satelliteBars[i * 18 + 10] = barHeight - 1;
				satelliteBars[i * 18 + 11] = 0.0;

				// 5
				satelliteBars[i * 18 + 12] = x;
				satelliteBars[i * 18 + 13] = barHeight - 1;
				satelliteBars[i * 18 + 14] = 0.0;

				// 6
				satelliteBars[i * 18 + 15] = x;
				satelliteBars[i * 18 + 16] = barHeight - 1 - signalHeight;
				satelliteBars[i * 18 + 17] = 0.0;

				// TODO - this should be optimized and passed to opengl in one call.
				sprintf(strbuf, "%d", sat -> prn);
				x += 5 - stringWidth(strbuf) / 2;

				glTranslatef(x, barHeight, 0);
				drawString(strbuf, .7 + .3 * (i % 2), .7 + .3 * (i % 2), .7 + .3 * (i % 2), TRUE);
				glTranslatef(-x, -barHeight, 0);

			} else {
				for (j = 0; j < 8; j++) {
					satelliteBarOutlineColors[i * 32 + j * 4] = 0.4;
					satelliteBarOutlineColors[i * 32 + j * 4 + 1] = 0.4;
					satelliteBarOutlineColors[i * 32 + j * 4 + 2] = 0.4;
					satelliteBarOutlineColors[i * 32 + j * 4 + 3] = 0.5;
					if (j < 6) {
						satelliteBarColors[i * 24 + j * 4] = 0.4;
						satelliteBarColors[i * 24 + j * 4 + 1] = 0.4;
						satelliteBarColors[i * 24 + j * 4 + 2] = 0.4;
						satelliteBarColors[i * 24 + j * 4 + 3] = 0.5;
					}
				}
			}
		}
	}
#endif

	glBindTexture(GL_TEXTURE_2D, 0);
	glEnableClientState(GL_COLOR_ARRAY);
#ifdef N900
	if (device -> satellites && device -> satellites -> len > 0) {
		glColorPointer(4, GL_FLOAT, 0, satelliteBarColors);
		glVertexPointer(3, GL_FLOAT, 0, satelliteBars);
		glDrawArrays(GL_TRIANGLES, 0, 6 * device -> satellites -> len);
	}
#endif
	glColorPointer(4, GL_FLOAT, 0, satelliteBarOutlineColors);
	glVertexPointer(3, GL_FLOAT, 0, satelliteBarOutlines);
	glDrawArrays(GL_LINES, 0, 8 * MAX_SATTELITES);
	glDisableClientState(GL_COLOR_ARRAY);
}

void drawConsoleLine(ConsoleLine* line) {
	drawString(line -> text, line -> r, line -> g, line -> b, TRUE);
}


void drawNavigationInstruction(Orientation orientation) {
	if (canvas.navigationStatus.currentDirection == NULL) {
		return;
	}
	int meters = canvas.navigationStatus.roundedDistanceToNextDirection;
	if (meters >= 100000) {
		sprintf(strbuf, "In %dkm: %s", (meters / 1000), canvas.navigationStatus.currentDirection -> text);
	} else if (meters >= 1000) {
		sprintf(strbuf, "In %.2fkm: %s", (meters / 1000.0), canvas.navigationStatus.currentDirection -> text);
	} else {
		sprintf(strbuf, "In %dm: %s", meters, canvas.navigationStatus.currentDirection -> text);
	}
	int textWidth = stringWidth(strbuf);

	meters = canvas.navigationStatus.distanceToTarget;
	if (meters >= 10000) {
		sprintf(strbuf2, "Distance to destination %dkm", meters / 1000);
	} else if (meters >= 1000) {
		sprintf(strbuf2, "Distance to destination %.2fkm", meters / 1000.0);
	} else {
		sprintf(strbuf2, "Distance to destination %d%s", meters, "m");
	}
	int textWidth2 = stringWidth(strbuf);

	if (textWidth2 > textWidth) {
		textWidth = textWidth2;
	}
	glDisable(GL_TEXTURE_2D);
	setBoxSize(textWidth + 0, 80, boxVertices);
	glPushMatrix();
	if (orientation == LANDSCAPE) {
		glTranslatef(SCREEN_WIDTH / 2 - textWidth / 2 - 100, 80, 0);
	} else {
		glTranslatef(SCREEN_HEIGHT / 2 - textWidth / 2 - 30, 55, 0);
	}

	glColor4f(1, 1, 1, 1);
	glVertexPointer(3, GL_FLOAT, 0, boxVertices);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glColor4f(.4, .4, .4, .7);

	if (orientation == LANDSCAPE) {
		glTranslatef(1, 0, 0);
	} else {
		glTranslatef(1, 1, 0);
	}
	setQuadStripSize(textWidth + 39, 40, quadStripVertices);

	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glEnable(GL_TEXTURE_2D);
	glTranslatef(20, 0, 0);
	drawString(strbuf, 1, 1, 1, FALSE);
	glTranslatef(0, 20, 0);
	drawString(strbuf2, 1, 1, 1, FALSE);
	glPopMatrix();
}

void drawScale(Orientation orientation) {
	double groundResolution = getGroundResolution(&canvas.center, canvas.zoom) / canvas.scale;
	double exponent = log10(groundResolution);
	double nextPowerOfTen = pow(10, floor(exponent) + 1);
	double norm = groundResolution / nextPowerOfTen;
	double scaleWidth;

	if (norm < 0.2) {
		scaleWidth = 20;
	} else if (norm < 0.5) {
		scaleWidth = 50;
	} else {
		scaleWidth = 100;
	}

	int scale = (int) scaleWidth * (int) nextPowerOfTen;

	glDisable(GL_TEXTURE_2D);
	scaleWidth = scaleWidth / norm;
	setBoxSize((int) scaleWidth, 14, boxVertices);
	glPushMatrix();
	if (orientation == LANDSCAPE) {
		glTranslatef(-80, 50, 0);
	} else {
		glTranslatef(13, 65, 0);
	}

	glColor4f(1, 1, 1, 1);
	glVertexPointer(3, GL_FLOAT, 0, boxVertices);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glColor4f(.4, .4, .4, .7);

	if (orientation == LANDSCAPE) {
		glTranslatef(1, 0, 0);
	} else {
		glTranslatef(1, 1, 0);
	}
	setQuadStripSize((int) scaleWidth - 1, 13, quadStripVertices);

	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (scale == 0) {
		sprintf(strbuf, "%2.3f m", (int)scaleWidth * nextPowerOfTen);
	} else if (scale >= 10000) {
		sprintf(strbuf, "%d km", scale / 1000);
	} else {
		sprintf(strbuf, "%d m", scale);
	}
	glEnable(GL_TEXTURE_2D);
	glTranslatef(scaleWidth/2 - stringWidth(strbuf)/2, -2, 0);
	drawString(strbuf, 1, 1, 1, FALSE);
	glPopMatrix();
}

void drawStatusBar(Orientation orientation) {
	if (canvas.orientationTransition > 0.001 && canvas.orientationTransition < .999) {
		if (orientation == LANDSCAPE) {
			glTranslatef((int) (-SCREEN_WIDTH * canvas.orientationTransition), 0, 0);
		} else {
			glTranslatef(0, (int) (SCREEN_HEIGHT * canvas.orientationTransition), 0);
		}
	} else {
		if (orientation != options.orientation) {
			return;
		}
		if (orientation == PORTRAIT) {
			glTranslatef(0, SCREEN_HEIGHT * canvas.orientationTransition, 0);
		}
	}
	if (orientation == PORTRAIT) {
		glRotatef(-90, 0.0, 0.0, 1.0);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	int glerror;
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 13 %d", glerror);
	}
	glColor4f(.4, .4, .4, 0.5);

	GLfloat statusBarHeight;

	if (orientation == LANDSCAPE) {
		statusBarHeight = 60;
	} else {
		statusBarHeight = 70;
	}

	// statusbar background - half transparent grey rectangle
	setQuadStripSize(SCREEN_WIDTH, statusBarHeight, quadStripVertices);
	glBindTexture(GL_TEXTURE_2D, 0);
	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (orientation == LANDSCAPE) {
		tasks -> x = 16;
		tasks -> y = 0;
		drawUiElement(tasks);

		closeApp -> x = SCREEN_WIDTH - 80;
		closeApp -> y = 0;
		drawUiElement(closeApp);

		if (canvas.busy) {
			busy -> x = SCREEN_WIDTH - 90;
			busy -> y = 30;
			drawUiElement(busy);
		}
	} else {
		if (canvas.busy) {
			busy -> x = SCREEN_HEIGHT - 20;
			busy -> y = 30;
			drawUiElement(busy);
		}
	}

	glTranslatef(10 + (orientation == LANDSCAPE ? 90 : 0), 5 + (orientation == LANDSCAPE ? 5 : 0), 0);

	if (consoleLines != NULL && consoleLines -> length > 0) {
		drawConsoleLine(g_queue_peek_tail(consoleLines));
	} else {
		sprintf(strbuf, "D/l: %.2f kb", downloaded / 1024.0);
		drawString(strbuf, 1.0, 1.0, 1.0, FALSE);
	}

	glTranslatef(0, 20, 0);
	drawNavigationInstruction(orientation);
	drawScale(orientation);
#ifdef N900
	switch (device -> fix -> mode) {
		case LOCATION_GPS_DEVICE_MODE_2D:
		sprintf(strbuf, "GPS fix: 2D");
		break;
		case LOCATION_GPS_DEVICE_MODE_3D:
		sprintf(strbuf, "GPS fix: 3D");
		break;
		case LOCATION_GPS_DEVICE_MODE_NOT_SEEN:
		sprintf(strbuf, "GPS fix: not seen");
		break;
		case LOCATION_GPS_DEVICE_MODE_NO_FIX:
		sprintf(strbuf, "GPS fix: no fix");
		break;
	}
#else
	sprintf(strbuf, "No GPS");
#endif
	drawString(strbuf, 1.0, 1.0, 1.0, FALSE);
	if (orientation == LANDSCAPE) {
		glTranslatef(140, -20, 0);
	} else {
		glTranslatef(120, -20, 0);
	}

	if (consoleLines != NULL && consoleLines -> length > 0) {

	} else {
#ifdef N900
		if (device -> satellites_in_use > 0 && device -> fix -> fields | LOCATION_GPS_DEVICE_ALTITUDE_SET && !isnan(device -> fix -> altitude)) {

#else
		if (FALSE) {
#endif
#ifdef N900
			sprintf(strbuf, "Altitude: %.2f m", device -> fix -> altitude);
#endif
		} else {
			sprintf(strbuf, "Altitude: ---");
		}
		drawString(strbuf, 1.0, 1.0, 1.0, FALSE);
	}
	glTranslatef(0, 20, 0);

	sprintf(strbuf, "Speed: ---");
#ifdef N900
	if (device -> satellites_in_use > 0 && device -> fix -> fields | LOCATION_GPS_DEVICE_SPEED_SET && !isnan(device -> fix -> speed)) {
		sprintf(strbuf, "Speed: %.2f km/h", device -> fix -> speed);
	}
#endif

	drawString(strbuf, 1.0, 1.0, 1.0, FALSE);

	if (orientation == LANDSCAPE) {
		glTranslatef(350, -20, 0);
	} else {
		glTranslatef(-120, 20, 0);
	}

	sprintf(strbuf, "FPS: %.1f", fps);
	drawString(strbuf, 1.0, 1.0, 1.0, FALSE);

	if (orientation == LANDSCAPE) {
		glTranslatef(0, 20, 0);
	} else {
		glTranslatef(120, 0, 0);
	}

	sprintf(strbuf, "Battery: %d%%", batteryPercentage);
	drawString(strbuf, 1.0, 1.0, 1.0, FALSE);

	if (orientation == LANDSCAPE) {
		glTranslatef(-230, -23, 0);
	} else {
		glTranslatef(130, -30, 0);
	}

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 14 %d", glerror);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	drawSatelliteBars(32, 12, 6);

	if (orientation == LANDSCAPE) {
		glTranslatef(SCREEN_WIDTH * canvas.orientationTransition, 0, 0);
	}
}

int processUiMouse() {
	GList* elem = uiElems;
	if (canvas.zoomBarAlpha > 0.2) {
		zoomKnot -> status = UI_SHOWN;
	} else {
		zoomKnot -> status = UI_HIDDEN;
	}
	int result = FALSE;

	if (canvas.orientationTransitionLinear < 80) {
		tasks -> status = UI_SHOWN;
		closeApp -> status = UI_SHOWN;
	}

	if (mouse.button == 1) {
		if (mouse.oldButton == 0) {
			while (elem != NULL) {
				UiElement* e = (UiElement*) elem->data;
				if (e -> status == UI_SHOWN && mouse.x >= e->x && mouse.x <= e->x + e->texture.size && mouse.y >= e->y && mouse.y <= e->y + e->texture.size) {
					pressedUiElem = e;
					if (pressedUiElem -> handlePressed != NULL) {
						pressedUiElem -> handlePressed();
						result = TRUE;
					}
					break;
				}

				elem = elem -> next;
			}
			if (result == FALSE) {
				pressedUiElem = NULL;
			}
		} else {
			if (pressedUiElem != NULL) {
				if (pressedUiElem -> handleDragged != NULL) {
					pressedUiElem -> handleDragged();
				}
				result = TRUE;
			}
		}
	}

	tasks -> status = UI_HIDDEN;
	closeApp -> status = UI_HIDDEN;

	return result;
}

void updateAnimationValue(int enabled, GLfloat *value, GLfloat minValue, GLfloat maxValue, GLfloat speed) {
	if (enabled) {
		if (*value < maxValue * (1.0 - speed * speed)) {
			*value += speed * (maxValue - *value);
		} else {
			*value = maxValue;
		}
	} else {
		if (*value > minValue + (maxValue - minValue) * speed * speed) {
			*value -= speed * (*value - minValue);
		} else {
			*value = minValue;
		}
	}
}

void updateUi() {

#ifdef N900
	if (device && device -> fix) {
//	if (device -> fix -> fields | LOCATION_GPS_DEVICE_LATLONG_SET) {
		if (device -> satellites_in_use > 5 && device -> fix -> fields | LOCATION_GPS_DEVICE_LATLONG_SET) {
#endif
//printf("fix latlong: %u\n", LOCATION_GPS_DEVICE_LATLONG_SET);
	position -> status = UI_SHOWN; //problem here: status is always UI_SHOWN
#ifdef N900
	canvas.currentPos.latitude = device -> fix -> latitude;
	canvas.currentPos.longitude = device -> fix -> longitude;
#endif
	TileCoordinate tc, tc2;
	toTileCoordinate(&canvas.currentPos, &tc, canvas.zoom);

	// gps position distance from screen center
	canvasCenterToTileCoordinate(&tc2);
	GLfloat dx = (tc.tilex - tc2.tilex) * TILE_SIZE + tc.x - tc2.x;
	GLfloat dy = (tc.tiley - tc2.tiley) * TILE_SIZE + tc.y - tc2.y;

	// canvas can be rotated, so we have to rotate distance vector
	GLfloat dx2 = dx * cos(canvas.rotz * M_PI / 180.0) - dy * sin(canvas.rotz * M_PI / 180.0);
	GLfloat dy2 = dx * sin(canvas.rotz * M_PI / 180.0) + dy * cos(canvas.rotz * M_PI / 180.0);

	dx2 *= canvas.scale;
	dy2 *= canvas.scale;

	GLfloat absdx = fabs(dx2);
	GLfloat absdy = fabs(dy2);

	if (absdx >= SCREEN_WIDTH / 2 || absdy >= SCREEN_HEIGHT / 2) {

		// to be safe on division
		if (absdx < 0.01) {
			absdx = 0.01;
		}
		if (absdy < 0.01) {
			absdy = 0.01;
		}

		// determine if arrow will be on x or y axis
		if (absdy / absdx < (GLfloat) SCREEN_HEIGHT / (GLfloat) SCREEN_WIDTH) {
			if (dx2 < 0) {
				canvas.arrowPosX = -SCREEN_WIDTH / 2;
				canvas.arrowPos = 1; // left
			} else {
				canvas .arrowPosX = SCREEN_WIDTH / 2;
				canvas.arrowPos = 2; // left
			}
			if (dy2 < 0) {
				canvas.arrowPosY = -(SCREEN_WIDTH / 2 * absdy / absdx);
			} else {
				canvas.arrowPosY = (SCREEN_WIDTH / 2 * absdy / absdx);
			}
		} else {
			if (dx2 < 0) {
				canvas.arrowPosX = -(SCREEN_HEIGHT / 2 * absdx / absdy);
			} else {
				canvas.arrowPosX = +(SCREEN_HEIGHT / 2 * absdx / absdy);
			}
			if (dy2 < 0) {
				canvas.arrowPosY = -SCREEN_HEIGHT / 2;
				canvas.arrowPos = 3; // up
			} else {
				canvas.arrowPosY = SCREEN_HEIGHT / 2;
				canvas.arrowPos = 4; // up
			}
		}
		//		fprintf(stderr, "%.2f, %.2f\n", canvas.arrowPosX, canvas.arrowPosY);

	} else {
		canvas.arrowPos = 0;
	}
	Point p;
	toScreenCoordinate(&tc, &p);
	position -> x = p.x - position -> texture.size / 2 / canvas.scale;
	position -> y = p.y - position -> texture.size / 2 / canvas.scale;

	position -> color.a = 0.75 + 0.25 * sin(nowMillis / 500.0);
	position -> color.r = position -> color.a;
	position -> color.g = position -> color.a;

#ifdef N900
	if (device -> satellites_in_use > 0) {
		position -> color.b = 1.0 - position -> color.a;
	} else {
#endif
	position -> color.b = position -> color.a;
#ifdef N900
}
} else {
position -> status = UI_HIDDEN;
canvas.arrowPosX = -1;
canvas.arrowPosY = -1;
}
}
#endif

	gotomypos -> color.r = canvas.followingMypos * .3 + .4;
	gotomypos -> color.g = canvas.followingMypos * .3 + .4;
	gotomypos -> color.b = canvas.followingMypos * .3 + .4;

	updateAnimationValue(canvas.searchBarActive, &canvas.searchBarY, 0.0, 64.0, 0.1);
	updateAnimationValue(canvas.zoomBarActive, &canvas.zoomBarAlpha, 0.0, 1.0, 0.03);

	// portrait/landscape orientation animation code:
	GList * volatileElem = volatileUiElems;
	if (canvas.orientationTransitionLinear > 0 && canvas.orientationTransitionLinear < 160) {
		int i = 0;

		if (canvas.orientationTransitionLinear == 80) {
			setZoomBarInitialized(FALSE);
		}

		if (canvas.searchBarActive) {
			if (canvas.orientationTransitionLinear <= 80) {
				canvas.searchBarY = 64 - canvas.orientationTransitionLinear;
			} else {
				canvas.searchBarY = 64 - (160 - canvas.orientationTransitionLinear);
			}
			if (canvas.searchBarY < 0.0) {
				canvas.searchBarY = 0.0;
			}
		}
		while (volatileElem != NULL) {
			UiElement * elem = volatileElem -> data;

			if (canvas.orientationTransitionLinear <= 80) {
				elem -> x = elem -> landscapex;
				elem -> y = elem -> landscapey - canvas.searchBarY;
				if (canvas.orientationTransitionLinear > i) {
					GLfloat xxx = canvas.orientationTransitionLinear - i;
					elem -> y += pow((xxx), 1.65);
				}
				elem -> texCoords = texCoordsLandscape[0];
			} else {
				elem -> x = elem -> portraitx - canvas.searchBarY;
				elem -> y = elem -> portraity;
				elem -> texCoords = texCoordsPortrait[0];
				if (160 - canvas.orientationTransitionLinear > i) {
					GLfloat xxx = 160 - (canvas.orientationTransitionLinear) - i;
					elem -> x += pow(xxx, 1.65);
				}
			}
			volatileElem = volatileElem -> next;
			i += 10;
		}
	} else {
		while (volatileElem != NULL) {
			UiElement * elem = volatileElem -> data;
			if (options.orientation == LANDSCAPE) {
				elem -> x = elem -> landscapex;
				elem -> y = elem -> landscapey - canvas.searchBarY;
				elem -> texCoords = texCoordsLandscape[0];
			} else {
				elem -> x = elem -> portraitx - canvas.searchBarY;
				elem -> y = elem -> portraity;
				elem -> texCoords = texCoordsPortrait[0];
			}
			volatileElem = volatileElem -> next;
		}
	}

	if (nowMillis > canvas.zoomBarVisibleMilis) {
		canvas.zoomBarActive = FALSE;
	}

	if (canvas.orientationTransitionLinear == 79) {
		activateZoomBar();
		zoomKnot -> x = canvas.zoomKnotPosition;
		zoomKnot -> y = SCREEN_HEIGHT - 185 - canvas.searchBarY;
	} else if (canvas.orientationTransitionLinear == 80) {
		activateZoomBar();
		zoomKnot -> y = canvas.zoomKnotPosition;
		zoomKnot -> x = SCREEN_WIDTH - 185 - canvas.searchBarY;
	}

	zoomKnot -> color.r = canvas.zoomBarAlpha;
	zoomKnot -> color.g = canvas.zoomBarAlpha;
	zoomKnot -> color.b = canvas.zoomBarAlpha;
	zoomKnot -> color.a = 1.0;
	//    zoomKnot -> status = UI_HIDDEN;
	if (canvas.orientationTransitionLinear < 80) {
		zoomKnot -> x += (canvas.zoomKnotPosition - zoomKnot -> x) / 4.0;
		zoomKnot -> y = SCREEN_HEIGHT - 185 - canvas.searchBarY;
		zoomKnot -> texCoords = texCoordsLandscape[0];
	} else {
		zoomKnot -> x = SCREEN_WIDTH - 185 - canvas.searchBarY;
		zoomKnot -> y += (canvas.zoomKnotPosition - zoomKnot -> y) / 4.0;
		zoomKnot -> texCoords = texCoordsPortrait[0];
	}
}

