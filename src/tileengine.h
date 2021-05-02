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

#ifndef TILEENGINE_H_
#define TILEENGINE_H_

#include "main.h"

void initTileEngine();
void tileEngineGotomypos();

void tileEngineToggle2dRotation();
void shutdownTileEngine();
void tileEngine_computeBoundingTrapezoid();
void tileEngineToggleSearchBar();
void tileEngineZoomIn();
void tileEngineZoomOut();
void tileEngineViewMode2D();
void tileEngineViewMode3D();
void tileEngineProcessMouse();
void tileEngineZoomKnotPressed();
void tileEngineZoomKnotDragged();
void tileEngineProcessSearchResults(GList* searchResults);
void tileEngineProcessNewRoute(Route* searchResults);
void activateZoomBar();
void recreateTiles();
void tileEngineFreeRoute();
void tileEngineUpdateOptimizedRoute();

void tileEngineUpdateCoordinates();
void tileEngineUpdateMarkers();
void tileEngine_handleZoomTiles();
void tileEngine_computeTilesVisibility();
void updateSyncWaitTiles();

void tileEngineChangeOrientation(Orientation orientation);
int getKnotDestZoom();
int getCanvasShiftX();
int getCanvasShiftY();
double getGroundResolution(WorldCoordinate* c, int zoom);

GLfloat pointDistance(Point* p1, Point *p2);
int isZoomBarInitialized();
void setZoomBarInitialized(int value);
void tileEngineProcessAccelerometer();
void toWorldCoordinate(TileCoordinate* tc, WorldCoordinate* result);
void toTileCoordinate(WorldCoordinate* wc, TileCoordinate* result, int zoom);
void toScreenCoordinate(TileCoordinate *tc, Point* p);
void fromScreenCoordinate(Point* p, TileCoordinate *tc);
void canvasCenterToTileCoordinate(TileCoordinate* tc);
double getDistance(WorldCoordinate* c1, WorldCoordinate* c2);
GLfloat getMarkerRotation();
void freePopupMenu();
#endif /* TILEENGINE_H_ */
