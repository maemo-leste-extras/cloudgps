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
#include <glib-2.0/glib.h>

#include "main.h"
#include "console.h"
#include "tile.h"
#include "uielement.h"
#include "texture.h"
#include "glunproject.h"
#include "downloadQueue.h"
#include "geocoding/geocoder.h"
#include "routing/router.h"
#include "markerIterator.h"
#include "geometry.h"

#ifdef N900
#define DOWNLOADING_THREADS 2
#else
#define DOWNLOADING_THREADS 4
#endif
extern Canvas canvas;
static UiElement* pressedUiElem;

SDL_Thread* downloadThreads[DOWNLOADING_THREADS];
volatile int downloadThreadBusy[DOWNLOADING_THREADS];

int downloadThread(void* queue);
void tileEngine_computeBoundingTrapezoid();
void tileEngine_computeTilesVisibility();

long lastZoomChange = 0;

int getCanvasShiftX() {
	return (TILES_X - (SCREEN_WIDTH / TILE_SIZE)) / 2;
}

int getCanvasShiftY() {
	return (TILES_Y - (SCREEN_HEIGHT / TILE_SIZE)) / 2;
}

void toQuadKey(TileCoordinate* c, char firstDigit, char* out) {
	int i = 0, zoom;
	for (zoom = c -> zoom; zoom > 0; zoom--) {
		char digit = firstDigit;
		int mask = 1 << (zoom - 1);

		if ((c -> tilex & mask) != 0) {
			digit++;
		}
		if ((c -> tiley & mask) != 0) {
			digit += 2;
		}

		out[i++] = (char) digit;
	}
	out[i] = '\0';
	//	fprintf(stderr, "zoom=%d, tilex=%d, tiley=%d, quadkey=%s\n", c->zoom, c->tilex, c->tiley, out);
}

void toScreenCoordinate(TileCoordinate *tc, Point* p) {
	p -> x = (tc->tilex - canvas.tilex - getCanvasShiftX()) * TILE_SIZE + tc->x;
	p -> y = (tc->tiley - canvas.tiley - getCanvasShiftY()) * TILE_SIZE + tc->y;
}
int adjustTileCoordinateShifts(TileCoordinate* tc);
// Warning: does not handle zoom
void fromScreenCoordinate(Point* p, TileCoordinate *tc) {
	tc -> tilex = 0;
	tc -> tiley = 0;
	tc -> x = p -> x + (canvas.tilex + getCanvasShiftX()) * TILE_SIZE;
	tc -> y = p -> y + (canvas.tiley + getCanvasShiftY()) * TILE_SIZE;
	adjustTileCoordinateShifts(tc);
}

/* Ground resolution (in meters per pixel) for given tile coordinate. */
double getGroundResolution(WorldCoordinate* c, int zoom) {
	int mapSize = TILE_SIZE << zoom;
	return (cos(c -> latitude * M_PI / 180.0) * 2.0 * M_PI * EARTH_RADIUS) / mapSize;
}

double inline degToRad(double deg) {
	return deg * M_PI / 180.0;
}

double getDistance(WorldCoordinate* c1, WorldCoordinate* c2) {
	double lat1 = degToRad(c1 -> latitude);
	double lat2 = degToRad(c2 -> latitude);
	double lon1 = degToRad(c1 -> longitude);
	double lon2 = degToRad(c2 -> longitude);

	return acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon2 - lon1)) * EARTH_RADIUS;
}

void toTileCoordinate(WorldCoordinate* wc, TileCoordinate* result, int zoom) {
	result -> zoom = zoom;

	double p = pow(2.0, zoom);
	double lat = (wc->longitude + 180.) / 360.0 * p;
	result -> tilex = (int) lat;
	result ->x = (lat - result->tilex) * TILE_SIZE;

	double lon = (1.0 - log(tan(wc->latitude * M_PI / 180.0) + 1.0 / cos(wc->latitude * M_PI / 180.0)) / M_PI) / 2.0 * p;
	result->tiley = (int) lon;
	result ->y = (lon - result->tiley) * TILE_SIZE;
}

void toWorldCoordinate(TileCoordinate* tc, WorldCoordinate* result) {
	double p = pow(2.0, tc->zoom);
	double xx = (double) tc->tilex + (tc->x / (double) TILE_SIZE);
	result->longitude = ((xx * 360.0) / p) - 180.0;

	double yy = tc->tiley + tc->y / (double) TILE_SIZE;
	double n = M_PI - 2.0 * M_PI * yy / p;
	double m = 0.5 * (exp(n) - exp(-n));
	double a = atan(m);
	result->latitude = (180.0 / M_PI * a);
}

void canvasCenterToTileCoordinate(TileCoordinate* tc) {
	tc -> zoom = canvas.zoom;
	tc -> tilex = canvas.tilex + getCanvasShiftX();
	tc -> tiley = canvas.tiley + getCanvasShiftY();
	tc -> x = (TILE_SIZE - canvas.x) + SCREEN_WIDTH / 2;
	tc -> y = (TILE_SIZE - canvas.y) + SCREEN_HEIGHT / 2;
}

t_tile* createTile(int i, int j) {
	t_tile * tile = (t_tile*) calloc(1, sizeof(t_tile));
	int mapSize = 1 << canvas.zoom;

	if (i >= 0 && (canvas.tilex + i < 0 || canvas.tiley + j < 0 || canvas.tilex + i >= mapSize || canvas.tiley + j >= mapSize)) {
		return NULL;
	}

	if (tile == NULL) {
		fprintf(stderr, "Memory allocation error in createTile()\n");
		exit(1);
	}
	tile->tilex = canvas.tilex + i;
	tile->tiley = canvas.tiley + j;
	tile->zoomHelper = 0;
	tile->zoom = canvas.zoom;
	tile->provider = canvas.provider;
	tile->stateChangeTime = -1000000;
	tile->textureUsedTime = -1000000;

	g_queue_push_tail(allCreatedTiles, tile);
	return tile;
}

void swapZoomTiles(TileCoordinate* tc1, TileCoordinate *tc2, int zoom, int destIdx);

void deleteZoomTile(TileCoordinate* tc, int zoomTileIdx, int zoom) {
	if (zoomTiles[zoom][zoomTileIdx] != NULL) {
		if (zoomTiles[zoom][zoomTileIdx] -> tilex != tc -> tilex || zoomTiles[zoom][zoomTileIdx] -> tiley != tc -> tiley || zoomTiles[zoom][zoomTileIdx] -> provider != canvas.provider) {
			if (zoomTileIdx == 1 && zoomTiles[zoom][0] == NULL) {
				swapZoomTiles(tc, tc, zoom, 0);
			} else {
				zoomTiles[zoom][zoomTileIdx] -> state = STATE_GARBAGE;
				zoomTiles[zoom][zoomTileIdx] -> deleted = TRUE;
				zoomTiles[zoom][zoomTileIdx] = NULL;
			}
		} else {
			if (zoomTiles[zoom][zoomTileIdx] -> state >= STATE_LOADED && zoomTiles[zoom][zoomTileIdx] -> texture != 0 && zoomTiles[zoom][zoomTileIdx] -> textureUsedTime > 0 && nowMillis
					- zoomTiles[zoom][zoomTileIdx] -> textureUsedTime > 3000) {
				GLuint texture = zoomTiles[zoom][zoomTileIdx] -> texture;
				zoomTiles[zoom][zoomTileIdx] -> texture = 0;
				zoomTiles[zoom][zoomTileIdx] -> state = STATE_GL_TEXTURE_NOT_CREATED;
				if (!zoomTiles[zoom][zoomTileIdx] -> textureShared) {
					deleteTexture(texture);
				}
				zoomTiles[zoom][zoomTileIdx] -> textureShared = FALSE;
				zoomTiles[zoom][zoomTileIdx] -> textureUsedTime = 0;
			}
		}
	}
}

long lastZoomHelperTileCreatedMilis = 0;
void createZoomTile(TileCoordinate* tc, int zoomTileIdx, int zoom) {
	if (zoomTiles[zoom][zoomTileIdx] == NULL) {
		int newTile = TRUE;

		// If canvas is moving downloading higher zoom should be avoided.
		if (canvas.attractionToPoint == 0) {
			if ((fabs(canvas.dx) > 0.5 || fabs(canvas.dy) > 0.5) && tc -> zoom > canvas.zoom) {
				newTile = FALSE;
			}

			// Limit amount of helper tiles added to queue per second.
			if (canvas.destScale != 0.0) {
				if (nowMillis - lastZoomHelperTileCreatedMilis < 200) {
					newTile = FALSE;
				}
			}
		}

		if (tc -> zoom > canvas.zoom + 3) {
			newTile = FALSE;
		}
		if (tc -> zoom < canvas.provider -> minZoom || tc -> zoom > canvas.provider -> maxZoom) {
			newTile = FALSE;
		}

		if (newTile) {
			zoomTiles[zoom][zoomTileIdx] = createTile(-1, -1);
			zoomTiles[zoom][zoomTileIdx] -> state = STATE_QUEUED;
			zoomTiles[zoom][zoomTileIdx] -> visible = FALSE;
			zoomTiles[zoom][zoomTileIdx] -> zoomHelper = TRUE;
			zoomTiles[zoom][zoomTileIdx] -> tilex = tc -> tilex;
			zoomTiles[zoom][zoomTileIdx] -> tiley = tc -> tiley;
			zoomTiles[zoom][zoomTileIdx] -> zoom = tc -> zoom;
			enqueue(downloadQueue, zoomTiles[zoom][zoomTileIdx]);
			lastZoomHelperTileCreatedMilis = nowMillis;
		}
	}
}

void computeZoomTileCoordinates(TileCoordinate* dest, TileCoordinate *maxZoom, int zoom) {
	float divisor = pow(2.0, MAX_ZOOM_HELPER_TILES - zoom - 1);
	dest -> tilex = maxZoom -> tilex / divisor;
	dest -> tiley = maxZoom -> tiley / divisor;
	dest -> zoom = zoom;
}

int compareCoords(TileCoordinate* tc, t_tile *tile) {
	if (tc -> tilex == tile -> tilex && tc -> tiley == tile -> tiley) {
		return TRUE;
	}
	return FALSE;
}

void swapZoomTiles(TileCoordinate* tc1, TileCoordinate *tc2, int zoom, int destIdx) {
	if (zoomTiles[zoom][1 - destIdx] == NULL) {
		return;
	}
	TileCoordinate* tc[2] = { tc1, tc2 };

	if (compareCoords(tc[destIdx], zoomTiles[zoom][1 - destIdx])) {
		if (zoomTiles[zoom][destIdx] != NULL) {
			zoomTiles[zoom][destIdx] -> deleted = 1;
			zoomTiles[zoom][destIdx] -> state = STATE_GARBAGE;
		}
		zoomTiles[zoom][destIdx] = zoomTiles[zoom][1 - destIdx];
		zoomTiles[zoom][1 - destIdx] = NULL;
	}
}

void tileEngine_handleZoomTiles() {
	int zoom, i, j, zoomTileIdx = 0;
	TileCoordinate canvasCenter, canvasCenterMaxZoom, attractionMaxZoom, tc1, tc2;
	WorldCoordinate wc;

	toTileCoordinate(&canvas.attraction, &attractionMaxZoom, (MAX_ZOOM_HELPER_TILES - 1));
	canvasCenterToTileCoordinate(&canvasCenter);
	toWorldCoordinate(&canvasCenter, &wc);
	toTileCoordinate(&wc, &canvasCenterMaxZoom, (MAX_ZOOM_HELPER_TILES - 1));

	int destZoomIdx = canvas.attractionToPoint;
	if (canvasCenterMaxZoom.tilex == attractionMaxZoom.tilex && canvasCenterMaxZoom.tiley == attractionMaxZoom.tiley) {
		destZoomIdx = 0;
	}

	for (zoom = 0; zoom < MAX_ZOOM_HELPER_TILES; zoom++) {
		computeZoomTileCoordinates(&tc1, &canvasCenterMaxZoom, zoom);
		computeZoomTileCoordinates(&tc2, &attractionMaxZoom, zoom);
		swapZoomTiles(&tc1, &tc2, zoom, destZoomIdx);
		deleteZoomTile(&tc1, 0, zoom);
		deleteZoomTile(&tc2, 1, zoom);
		createZoomTile(destZoomIdx == 1 ? &tc2 : &tc1, destZoomIdx, zoom);
	}

	// Go through all currently displayed tiles and set oldtile where applicable.
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			if (tiles[i][j][currentTilesIdx] != NULL && tiles[i][j][currentTilesIdx] -> visible && tiles[i][j][currentTilesIdx] -> texture == 0) {
				if (tiles[i][j][currentTilesIdx] -> texture == 0 /*&& tiles[i][j][currentTilesIdx] -> oldTiles[0] == NULL*/&& tiles[i][j][currentTilesIdx] -> visible == TRUE) {
					int k, l = 1;
					k = tiles[i][j][currentTilesIdx] -> zoom;

					//if (canvas.destScale == 0.0 && canvas.scale != 1.0) {
					k = tiles[i][j][currentTilesIdx] -> zoom - 1;
					l = 2;
					//}

					if (k <= 0) {
						k = 1;
						l = 2;
					}

					while (k >= 0) {
						if (k < MAX_ZOOM_HELPER_TILES) {
							for (zoomTileIdx = 0; zoomTileIdx < 2; zoomTileIdx++) {
								if (zoomTiles[k][zoomTileIdx] != NULL) {
									if ((tiles[i][j][currentTilesIdx] -> tilex / l == zoomTiles[k][zoomTileIdx] -> tilex) && (tiles[i][j][currentTilesIdx] -> tiley / l
											== zoomTiles[k][zoomTileIdx] -> tiley)) {
										if (zoomTiles[k][zoomTileIdx] -> state >= STATE_LOADED) {
											if (zoomTiles[k][zoomTileIdx] -> texture == 0) {
												if (texturesCreatedThisFrame >= MAX_TEXTURES_CREATED_PER_FRAME) {
													continue;
												}
												zoomTiles[k][zoomTileIdx] -> texture = createTexture(zoomTiles[k][zoomTileIdx] -> pixels4444, TILE_SIZE, TILE_SIZE, TRUE);
												zoomTiles[k][zoomTileIdx] -> state = STATE_LOADED;
												texturesCreatedThisFrame++;
											}
											t_tile* oldTile = (t_tile*) tiles[i][j][currentTilesIdx] -> oldTiles[0];
											if (oldTile == NULL || (oldTile != NULL && oldTile -> zoom <= k) || (oldTile != NULL && oldTile -> provider != canvas.provider)) {
												tiles[i][j][currentTilesIdx] -> oldTiles[0] = zoomTiles[k][zoomTileIdx];
											}
											if (oldTile != NULL && oldTile -> zoom >= k) {
												goto found;
											}
										}
										if (l < 3 && !isOutsideCanvas(zoomTiles[k][zoomTileIdx]) && tiles[i][j][currentTilesIdx] -> state == STATE_QUEUE_DELAYED) {
											tiles[i][j][currentTilesIdx] -> state = STATE_LOADED;
										}
										// This will be return from function someday...
										goto found;
									}
								}
							}
						}

						l *= 2;
						k--;
					}
				}

				found:

				// If the queueing was delayed enqueue now.
				if (tiles[i][j][currentTilesIdx] -> state == STATE_QUEUE_DELAYED && nowMillis - lastZoomChange > 1500) {
					if (canvas.attractionZooming == 0 || (canvas.attractionZooming != 0 && canvas.zoom == canvas.attractionDestZoom)) {
						tiles[i][j][currentTilesIdx] -> state = STATE_QUEUED;
						enqueue(downloadQueue, tiles[i][j][currentTilesIdx]);
					}
				}
			}
		}
	}
}

void initTileEngine() {
	int i, j;
	allCreatedTiles = g_queue_new();
	downloadQueue = createQueue();
	canvas.attractionZooming = 0;
	canvas.attractionDestZoom = -1;
	canvas.attractionIntermediateZoom = -1;
	canvas.scale = 1.0;
	canvas.friction = .97;
	canvas.viewMode = VIEW_2D;
#ifndef N900
	canvas.currentPos.latitude = 50.0618;
	canvas.currentPos.longitude = 19.9373;
#endif

	for (i = 0; i < DOWNLOADING_THREADS; i++) {
		//        fprintf(stderr, "before SDL_CreateThread\n");
		downloadThreads[i] = SDL_CreateThread(downloadThread, (void*) &downloadThreadBusy[i]);
		//        fprintf(stderr, "after SDL_CreateThread\n");
		//		SDL_Delay(15);
	}

	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			tiles[i][j][currentTilesIdx] = createTile(i, j);
		}
	}
	tileEngine_computeBoundingTrapezoid();
	tileEngine_computeTilesVisibility();
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			if (tiles[i][j][currentTilesIdx] != NULL) {
				enqueue(downloadQueue, tiles[i][j][currentTilesIdx]);
			}
		}
	}
	tileEngine_handleZoomTiles();
	addConsoleLine("CloudGPS ver. " CLOUDGPS_VERION, 0, 1, 0);
}

int inline roundDown(double x) {
	return x < 0 ? (int) (x - 1.0) : (int) x;
}

void deallocateSearchMarkers() {
	GList* list = canvas.markers;

	while (list != NULL) {
		ScreenMarker* marker = (ScreenMarker*) list -> data;
		free(marker -> geoCodeResult);
		list = list -> next;
		canvas.markers = g_list_remove(canvas.markers, marker);
	}
}

ScreenMarker* createMarker(GeoCodeResult* geoCodeResult, MarkerType type) {
	ScreenMarker* marker = calloc(1, sizeof(ScreenMarker));
	if (marker != NULL) {
		marker -> type = type;
		marker -> tileCoordinate.zoom = -1;
		marker -> visible = TRUE;
		marker -> raised = FALSE;
		marker -> raiseLevel = 50;
		marker -> geoCodeResult = geoCodeResult;
	}
	return marker;
}

void addSearchMarkers(GList* searchResults) {
	while (searchResults != NULL) {
		GeoCodeResult* geoCodeResult = (GeoCodeResult*) searchResults -> data;
		ScreenMarker* marker = createMarker(geoCodeResult, SEARCH_RESULT);
		canvas.markers = g_list_prepend(canvas.markers, marker);
		searchResults = searchResults -> next;
	}
}

void setAttractionToPoint(WorldCoordinate* wc) {
	canvas.attraction.latitude = wc -> latitude;
	canvas.attraction.longitude = wc -> longitude;
	canvas.followingMypos = 0;
	canvas.attractionZooming = 0;
	canvas.attractionToPoint = 1;
}

void setAttractionToSearchResults() {
	if (g_list_length(canvas.markers) == 1) {
		ScreenMarker* marker = canvas.markers -> data;
		setAttractionToPoint(&marker -> geoCodeResult -> centroid);
		canvas.searchBarActive = 0;
	} else if (g_list_length(canvas.markers) > 1) {
		WorldCoordinate worldUpperLeft, worldLowerRight;
		GList* list = canvas.markers;
		ScreenMarker* marker = list -> data;
		worldUpperLeft = marker -> geoCodeResult -> centroid;
		worldLowerRight = marker -> geoCodeResult -> centroid;
		list = list -> next;

		while (list != NULL) {
			marker = list -> data;
			WorldCoordinate wc = marker -> geoCodeResult -> centroid;
			if (wc.latitude < worldUpperLeft.latitude) {
				worldUpperLeft.latitude = wc.latitude;
			}
			if (wc.longitude < worldUpperLeft.longitude) {
				worldUpperLeft.longitude = wc.longitude;
			}

			if (wc.latitude > worldLowerRight.latitude) {
				worldLowerRight.latitude = wc.latitude;
			}
			if (wc.longitude > worldLowerRight.longitude) {
				worldLowerRight.longitude = wc.longitude;
			}
			list = list -> next;
		}

		int zoom = canvas.provider -> maxZoom;

		while (zoom > canvas.provider -> minZoom) {
			TileCoordinate tileUpperLeft, tileLowerRight;
			Point upperLeft, lowerRight;
			toTileCoordinate(&worldUpperLeft, &tileUpperLeft, zoom);
			toTileCoordinate(&worldLowerRight, &tileLowerRight, zoom);
			toScreenCoordinate(&tileUpperLeft, &upperLeft);
			toScreenCoordinate(&tileLowerRight, &lowerRight);

			if (fabs(upperLeft.x - lowerRight.x) < SCREEN_HEIGHT * 0.9 && fabs(upperLeft.y - lowerRight.y) < SCREEN_HEIGHT * 0.9) {
				break;
			}

			zoom--;
		}
		canvas.attractionDestZoom = zoom;
		WorldCoordinate destination;
		destination.latitude = (worldLowerRight.latitude + worldUpperLeft.latitude) / 2.0;
		destination.longitude = (worldLowerRight.longitude + worldUpperLeft.longitude) / 2.0;
		setAttractionToPoint(&destination);
		canvas.searchBarActive = 0;
	}
}

void tileEngineProcessSearchResults(GList* searchResults) {
	deallocateSearchMarkers();
	addSearchMarkers(searchResults);
	setAttractionToSearchResults();
}

void tileEngineFreeRoute() {
	if (canvas.route.directions != NULL) {
		g_array_free(canvas.route.directions, TRUE);
		canvas.route.directions = NULL;
	}
	if (canvas.route.optimizedRoute.lineStripTileCoordinates != NULL) {
		g_array_free(canvas.route.optimizedRoute.lineStripTileCoordinates, TRUE);
		canvas.route.optimizedRoute.lineStripTileCoordinates = NULL;
	}
	if (canvas.route.optimizedRoute.visibleLines != NULL) {
		g_list_free(canvas.route.optimizedRoute.visibleLines);
		canvas.route.optimizedRoute.visibleLines = NULL;
	}
	if (canvas.route.query != NULL) {
		if (canvas.route.query -> waypoints != NULL) {
			g_list_free(canvas.route.query -> waypoints);
			canvas.route.query -> waypoints = NULL;
		}
		free(canvas.route.query);
		canvas.route.query = NULL;
	}
	canvas.route.lengthMeters = 0;
}

void tileEngineUpdateOptimizedRoute() {
	if (canvas.route.optimizedRoute.lineStripZoom != canvas.zoom) {
		canvas.route.optimizedRoute.lineStripTileCoordinates = NULL;
	}
	if (canvas.route.directions == NULL) {
		return;
	}

	if (canvas.route.optimizedRoute.lineStripTileCoordinates == NULL) {
		int i, j;
		canvas.route.optimizedRoute.lineStripTileCoordinates = g_array_new(FALSE, FALSE, sizeof(TileCoordinate));
		if (canvas.route.optimizedRoute.lineStripTileCoordinates == NULL) {
			fprintf(stderr, "Memory allocation error in tileEngineRecalculateOptimizedRoute\n");
			exit(1);
		}
		for (i = 0; i < canvas.route.directions -> len; i++) {
			RouteDirection direction = g_array_index(canvas.route.directions, RouteDirection, i);
			for (j = 0; j < direction.polyLine -> len; j++) {
				WorldCoordinate wc = g_array_index(direction.polyLine, WorldCoordinate, j);
				TileCoordinate tc;
				toTileCoordinate(&wc, &tc, canvas.zoom);
				canvas.route.optimizedRoute.lineStripTileCoordinates = g_array_append_vals(canvas.route.optimizedRoute.lineStripTileCoordinates, &tc, 1);
			}
		}
		canvas.route.optimizedRoute.lineStripZoom = canvas.zoom;
	}
}

GeoCodeResult* createGeoCodeResult(WorldCoordinate* wc) {
	GeoCodeResult* r = calloc(1, sizeof(GeoCodeResult));
	r -> centroid = *wc;
	return r;
}

void addRouteMarker(WorldCoordinate* wc, MarkerType type) {
	GeoCodeResult* geoCode = createGeoCodeResult(wc);
	ScreenMarker* marker = createMarker(geoCode, type);
	canvas.routeMarkers = g_list_append(canvas.routeMarkers, marker);
}

void clearRouteMarkers() {
	if (canvas.routeMarkers != NULL) {
		g_list_free(canvas.routeMarkers);
		canvas.routeMarkers = NULL;
	}
}

void createRouteMarkers() {
	addRouteMarker(&canvas.route.query -> start, ROUTE_START);
	GList* waypoint = canvas.route.query -> waypoints;

	while (waypoint != NULL) {
		WorldCoordinate* wc = (WorldCoordinate*) waypoint -> data;
		addRouteMarker(wc, ROUTE_WAYPOINT);
		waypoint = waypoint -> next;
	}
	addRouteMarker(&canvas.route.query -> end, ROUTE_END);
}

void updateMarkerTileCoordinate(ScreenMarker* marker);
void updateMarkerScreenCoordinate(ScreenMarker* marker);
void updateRouteMarker(ScreenMarker* marker, WorldCoordinate* wc) {
	marker -> geoCodeResult -> centroid = *wc;
	marker -> recalculateCoords = TRUE;
	marker -> raised = 0;
	marker -> raiseLevel = 50;
	updateMarkerTileCoordinate(marker);
	updateMarkerScreenCoordinate(marker);
}

void updateRouteMarkers(MarkerType type) {
	GList* elem = canvas.routeMarkers;

	while (elem != NULL) {
		ScreenMarker* marker = elem -> data;
		if (marker -> type == type) {
			if (marker -> type == ROUTE_START) {
				updateRouteMarker(marker, &canvas.route.query -> start);
			} else if (marker -> type == ROUTE_END) {
				updateRouteMarker(marker, &canvas.route.query -> end);
			}
		}
		elem = elem -> next;
	}
}

void tileEngineProcessNewRoute(Route* route) {
	tileEngineFreeRoute();
	canvas.route = *route;
	memset(route, 0, sizeof(Route));
	if (canvas.routeMarkers == NULL) {
		createRouteMarkers();
	}
}

void updateMarkerTileCoordinate(ScreenMarker* marker) {
	if (marker -> tileCoordinate.zoom != canvas.zoom || marker -> recalculateCoords) {
		toTileCoordinate(&marker -> geoCodeResult -> centroid, &marker -> tileCoordinate, canvas.zoom);
		marker -> recalculateCoords = FALSE;
	}
}

void updateMarkerScreenCoordinate(ScreenMarker* marker) {
	toScreenCoordinate(&marker -> tileCoordinate, &marker -> screenCoordinate);
}

void updateMarkerRaiseLevel(ScreenMarker* marker) {
	if (marker -> visible) {
		if (marker -> raised) {
			if (marker -> raiseLevel < 30) {
				marker -> raiseLevel++;
			}
		} else {
			if (marker -> raiseLevel > 0) {
				marker -> raiseLevel--;
			}
		}
	}
}

void updateMarkerVisibility(ScreenMarker* marker) {
	int tilex = marker -> tileCoordinate.tilex;
	int tiley = marker -> tileCoordinate.tiley;
	if (tilex <= canvas.tilex + TILES_X && tilex >= canvas.tilex && tiley <= canvas.tiley + TILES_Y && tiley >= canvas.tiley) {
		marker -> visible = TRUE;
	} else {
		marker -> visible = FALSE;
	}
}

void tileEngineUpdateMarkerList(GList* list) {
	while (list != NULL) {
		ScreenMarker* marker = (ScreenMarker*) list -> data;
		updateMarkerTileCoordinate(marker);
		updateMarkerScreenCoordinate(marker);
		updateMarkerVisibility(marker);
		updateMarkerRaiseLevel(marker);
		list = list -> next;
	}
}

void updateBusyValue() {
	int i;
	canvas.busy = FALSE;

	for (i = 0; i < DOWNLOADING_THREADS; i++) {
		if (downloadThreadBusy[i]) {
			canvas.busy = TRUE;
		}
	}

	if (routingStatus != NO_QUERY) {
		canvas.busy = TRUE;
	}
	if (searchResultsStatus != NO_QUERY) {
		canvas.busy = TRUE;

	}
}

void tileEngineUpdateMarkers() {
	tileEngineUpdateMarkerList(canvas.markers);
	tileEngineUpdateMarkerList(canvas.routeMarkers);
}

int adjustTileCoordinateShifts(TileCoordinate* tc) {
	int horizontalShift = roundDown(tc -> x / (double) TILE_SIZE);
	int verticalShift = roundDown(tc -> y / (double) TILE_SIZE);

	tc -> x -= horizontalShift * TILE_SIZE;
	tc -> y -= verticalShift * TILE_SIZE;

	tc -> tilex += horizontalShift;
	tc -> tiley += verticalShift;
	if (horizontalShift != 0 || verticalShift != 0) {
		return TRUE;
	}
	return FALSE;

}
int adjustShifts() {
	int horizontalShift = roundDown(canvas.x / (double) TILE_SIZE);
	int verticalShift = roundDown(canvas.y / (double) TILE_SIZE);

	canvas.x -= horizontalShift * TILE_SIZE;
	canvas.y -= verticalShift * TILE_SIZE;

	canvas.tilex += horizontalShift;
	canvas.tiley += verticalShift;
	if (horizontalShift != 0 || verticalShift != 0) {
		return TRUE;
	}
	return FALSE;
}

void recreateTiles() {
	purgeQueue(downloadQueue);
	int i, j, k;

	// start loading new tile textures:
	syncLoadedTilesAppearance = TRUE;
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			tiles[i][j][1 - currentTilesIdx] = createTile(i, j);
			if (tiles[i][j][1 - currentTilesIdx] != NULL) {
				if (tiles[i][j][currentTilesIdx] != NULL) {
					if (tiles[i][j][currentTilesIdx] -> texture != 0) {
						tiles[i][j][1 - currentTilesIdx] -> oldTiles[0] = tiles[i][j][currentTilesIdx];
						tiles[i][j][1 - currentTilesIdx] -> stateChangeTime = nowMillis;
					} else {
						for (k = 0; k < 4; k++) {
							tiles[i][j][1 - currentTilesIdx] -> oldTiles[k] = tiles[i][j][currentTilesIdx]-> oldTiles[k];
						}
					}
				}
			}
		}
	}
	currentTilesIdx = 1 - currentTilesIdx;
	tileEngine_computeBoundingTrapezoid();
	tileEngine_computeTilesVisibility();
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			if (tiles[i][j][currentTilesIdx] != NULL) {
				enqueue(downloadQueue, tiles[i][j][currentTilesIdx]);
			}
		}
	}
}

void handleZoomChange() {
	// compute new tilex, tiley, x and y for canvas using canvas.center
	int i, j;

	TileCoordinate tc;
	toTileCoordinate(&canvas.center, &tc, canvas.zoom);

	int oldtilex = canvas.tilex;
	int oldtiley = canvas.tiley;

	canvas.tilex = tc.tilex - getCanvasShiftX();
	canvas.tiley = tc.tiley - getCanvasShiftY();
	canvas.x = tc.x - SCREEN_WIDTH / 2;
	canvas.y = tc.y - SCREEN_HEIGHT / 2;

	adjustShifts();

	if (canvas.x != 0.0) {
		canvas.x = TILE_SIZE - canvas.x;
	}
	if (canvas.y != 0.0) {
		canvas.y = TILE_SIZE - canvas.y;
	}

	syncLoadedTilesAppearance = TRUE;
	lastZoomChange = nowMillis;

	if (canvas.zoom - canvas.oldZoom == 1) {
		//        fprintf(stderr, "zoomIn 1\n");

		for (i = 0; i < TILES_X; i++) {
			for (j = 0; j < TILES_Y; j++) {
				tiles[i][j][1 - currentTilesIdx] = createTile(i, j);
				if (tiles[i][j][1 - currentTilesIdx] == NULL) {
					continue;
				}
				int oldi = tiles[i][j][1 - currentTilesIdx] -> tilex - (oldtilex * 2);
				int oldj = tiles[i][j][1 - currentTilesIdx] -> tiley - (oldtiley * 2);

				if (oldi >= 0 && oldj >= 0) {

					if (tiles[oldi / 2][oldj / 2][currentTilesIdx] != NULL && tiles[oldi / 2][oldj / 2][currentTilesIdx] -> state >= STATE_LOADED) {
						tiles[i][j][1 - currentTilesIdx] -> oldTiles[0] = tiles[oldi / 2][oldj / 2][currentTilesIdx];

						if (tiles[oldi / 2][oldj / 2][currentTilesIdx] -> state == STATE_GL_TEXTURE_NOT_CREATED) {
							tiles[oldi / 2][oldj / 2][currentTilesIdx] -> texture = createTexture(tiles[oldi / 2][oldj / 2][currentTilesIdx] -> pixels4444, TILE_SIZE, TILE_SIZE, TRUE);
						}

						tiles[oldi / 2][oldj / 2][currentTilesIdx] -> state = STATE_SCALED;
						tiles[i][j][1 - currentTilesIdx] -> transitionTime = options.zoomChangeFadeTime;
						if (canvas.attractionZooming == 0 || canvas.zoom == canvas.attractionDestZoom) {
							if (canvas.zoomBarLastDragged - nowMillis < 500) {
								tiles[i][j][1 - currentTilesIdx] -> state = STATE_QUEUE_DELAYED;
							} else {
								tiles[i][j][1 - currentTilesIdx] -> state = STATE_QUEUED;
								enqueue(downloadQueue, tiles[i][j][1 - currentTilesIdx]);
							}
						}
					} else {
						tiles[oldi / 2][oldj / 2][currentTilesIdx] = NULL;
						//						tiles[i][j][1 - currentTilesIdx] -> transitionTime
						//								= options.zoomChangeFadeTime;
						if (canvas.attractionZooming == 0 || (canvas.attractionZooming != 0 && canvas.zoom == canvas.attractionDestZoom)) {
							if (canvas.zoomBarLastDragged - nowMillis < 250) {
								tiles[i][j][1 - currentTilesIdx] -> state = STATE_QUEUE_DELAYED;
							} else {
								tiles[i][j][1 - currentTilesIdx] -> state = STATE_QUEUED;
								enqueue(downloadQueue, tiles[i][j][1 - currentTilesIdx]);
							}
						}
					}
				}
			}
		}
		currentTilesIdx = 1 - currentTilesIdx;
	} else if (canvas.zoom - canvas.oldZoom == -1) {
		for (i = 0; i < TILES_X; i++) {
			for (j = 0; j < TILES_Y; j++) {
				tiles[i][j][1 - currentTilesIdx] = createTile(i, j);
				if (tiles[i][j][1 - currentTilesIdx]) {
					if (canvas.attractionZooming == 0 || (canvas.attractionZooming != 0 && canvas.zoom == canvas.attractionDestZoom)) {
						if (canvas.zoomBarLastDragged - nowMillis < 250) {
							tiles[i][j][1 - currentTilesIdx] -> state = STATE_QUEUE_DELAYED;
						} else {
							tiles[i][j][1 - currentTilesIdx] -> state = STATE_QUEUED;
							enqueue(downloadQueue, tiles[i][j][1 - currentTilesIdx]);
						}
					}
					tiles[i][j][1 - currentTilesIdx] -> transitionTime = options.zoomChangeFadeTime;
				}
			}
		}
		currentTilesIdx = 1 - currentTilesIdx;
	} else {
		// Normally not used because changes to zoom level are supported only by one level at a time.
		// Useful for changing between tile providers that have have no zoom level common.
		recreateTiles();
		fprintf(stderr, "Warn: handleZoomChange: recreateTiles()\n");
	}
	forceGarbageCollection = TRUE;
}

int inline getLowerDeallocateBound(int size, int shift) {
	return shift < 0 ? 0 : (size - shift) < 0 ? 0 : (size - shift);
}

int inline getUpperDeallocateBound(int size, int shift) {
	return shift < 0 ? (-shift < size ? -shift : size) : size;
}

void deallocateTiles(int horizontalShift, int verticalShift) {

	int i, j;
	for (i = getLowerDeallocateBound(TILES_X, horizontalShift); i < getUpperDeallocateBound(TILES_X, horizontalShift); i++) {
		for (j = 0; j < TILES_Y; j++) {
			//            deallocateTile(tiles[i][j][currentTilesIdx]);
			if (tiles[i][j][currentTilesIdx] != NULL) {
				tiles[i][j][currentTilesIdx] -> deleted = TRUE;
				tiles[i][j][currentTilesIdx] -> state = STATE_GARBAGE;
			}

			tiles[i][j][currentTilesIdx] = NULL;
		}
	}
	for (i = 0; i < TILES_X; i++) {
		for (j = getLowerDeallocateBound(TILES_Y, verticalShift); j < getUpperDeallocateBound(TILES_Y, verticalShift); j++) {
			//            deallocateTile(tiles[i][j][currentTilesIdx]);
			if (tiles[i][j][currentTilesIdx] != NULL) {
				tiles[i][j][currentTilesIdx] -> deleted = TRUE;
				tiles[i][j][currentTilesIdx] -> state = STATE_GARBAGE;
			}
			tiles[i][j][currentTilesIdx] = NULL;
		}
	}
}

void updateTiles(int horizontalShift, int verticalShift) {
	deallocateTiles(horizontalShift, verticalShift);
	int i, j;
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			int newPosX = i - horizontalShift;
			int newPosY = j - verticalShift;
			if (newPosX >= 0 && newPosX < TILES_X && newPosY >= 0 && newPosY < TILES_Y) {
				tiles[i][j][1 - currentTilesIdx] = tiles[newPosX][newPosY][currentTilesIdx];
				tiles[newPosX][newPosY][currentTilesIdx] = NULL;
			} else {
				tiles[i][j][1 - currentTilesIdx] = createTile(i, j);
				if (canvas.attractionZooming == 0 || (canvas.attractionZooming != 0 && canvas.zoom == canvas.attractionDestZoom)) {
					if (tiles[i][j][1 - currentTilesIdx] != NULL) {
						enqueue(downloadQueue, tiles[i][j][1 - currentTilesIdx]);
					}
				}
			}
		}
	}
	currentTilesIdx = 1 - currentTilesIdx;
}

// FIXME - gives wrong results in 3d mode
void setClickedTileCoordinate(TileCoordinate* tc) {
	double x2 = (mouse.oldx - SCREEN_WIDTH / 2) * cos(-canvas.rotz * M_PI / 180.0) - (mouse.oldy - SCREEN_HEIGHT / 2) * sin(-canvas.rotz * M_PI / 180.0);
	double y2 = (mouse.oldx - SCREEN_WIDTH / 2) * sin(-canvas.rotz * M_PI / 180.0) + (mouse.oldy - SCREEN_HEIGHT / 2) * cos(-canvas.rotz * M_PI / 180.0);
	x2 /= canvas.scale;
	y2 /= canvas.scale;

	tc -> tilex = canvas.tilex + getCanvasShiftX();
	tc -> tiley = canvas.tiley + getCanvasShiftY();
	tc -> zoom = canvas.zoom;
	tc -> x = x2 + SCREEN_WIDTH / 2 + TILE_SIZE - canvas.x;
	tc -> y = y2 + SCREEN_HEIGHT / 2 + TILE_SIZE - canvas.y;

	adjustTileCoordinateShifts(tc);
}

GLfloat getMarkerRotation() {
	return -canvas.rotz - pow(canvas.orientationTransitionLinear / 160.0, 1.5) * 90.0;
}

extern Texture searchMarkTexture;
void rotate2d(GLfloat* x, GLfloat* y, double rotation);

ScreenMarker* findMarkerUnderMouse() {
	TileCoordinate tc;
	Point clicked;

	setClickedTileCoordinate(&tc);
	toScreenCoordinate(&tc, &clicked);
	GLfloat textureX, textureY;
	textureX = 0.0;
	textureY = searchMarkTexture.size / 2;
	rotate2d(&textureX, &textureY, getMarkerRotation());

	Point screenCoordinateClicked;

	screenCoordinateClicked.x = textureX + clicked.x;
	screenCoordinateClicked.y = textureY + clicked.y;

	ScreenMarker* closestMarker = NULL;

	ScreenMarker* marker = resetMarkerIterator();
	GLfloat minDistance = searchMarkTexture.size / 2;
	while (marker != NULL) {
		if (marker -> visible) {
			GLfloat distance = pointDistance(&marker -> screenCoordinate, &screenCoordinateClicked);

			if (distance < minDistance) {
				minDistance = distance;
				closestMarker = marker;
			}
		}
		marker = markerIteratorNext();
	}
	return closestMarker;
}

extern Uint8* keys;
void handleMouseClick() {
	ScreenMarker* marker = findMarkerUnderMouse();

	if (marker != NULL) {
		setAttractionToPoint(&marker -> geoCodeResult -> centroid);
		if (marker -> geoCodeResult -> description != NULL) {
			addConsoleLine((char *) marker -> geoCodeResult -> description, 0, 1, 1);
		}
	}
}

void handleMouseUp() {
	if (canvas.draggedMarker != NULL) {
		canvas.draggedMarker -> raised = 0;
		canvas.draggedMarker = NULL;
	}
	if (canvas.popup.popupEntries != NULL) {
		if (canvas.popup.clickedPopupEntry != NULL) {
			if (!canvas.popup.currentClickCreatedPopup) {
				canvas.popup.clickedPopupEntry -> handlePressed(canvas.popup.clickedPopupEntry -> argument);
				freePopupMenu();
			} else if (canvas.popup.animationFrame >= 30) {
				canvas.popup.clickedPopupEntry -> handlePressed(canvas.popup.clickedPopupEntry -> argument);
				freePopupMenu();
			}
		}

		if (canvas.popup.currentClickCreatedPopup) {
			canvas.popup.currentClickCreatedPopup = FALSE;
		} else {
			freePopupMenu();
		}
	}
	canvas.mouseHoldInPlace = -1;
}

void handleDraggedMarker() {
	//		canvas.draggedMarker -> screenCoordinate.x += mouse.xdelta;
	//		canvas.draggedMarker -> screenCoordinate.y += mouse.ydelta;
	GLfloat markerMoveX = mouse.xdelta / canvas.scale;
	GLfloat markerMoveY = mouse.ydelta / canvas.scale;

	GLfloat canvasScrollX = 0.0;
	GLfloat canvasScrollY = 0.0;

	if (mouse.x < 100) {
		canvasScrollX = .2;
	}

	if (mouse.x > SCREEN_WIDTH - 100) {
		canvasScrollX = -.2;
	}

	if (mouse.y < 100) {
		canvasScrollY = .2;
	}

	if (mouse.y > SCREEN_HEIGHT - 100) {
		canvasScrollY = -.2;
	}

	if (canvasScrollX != 0.0 || canvasScrollY != 0.0) {
		canvas.friction = 0.97;
		GLfloat rotatedScrollX = canvasScrollY * sin(canvas.rotz * M_PI / 180.0) + canvasScrollX * sin((canvas.rotz + 90) * M_PI / 180.0);
		GLfloat rotatedScrollY = canvasScrollY * cos(canvas.rotz * M_PI / 180.0) + canvasScrollX * cos((canvas.rotz + 90) * M_PI / 180.0);
		canvas.dx += rotatedScrollX;
		canvas.dy += rotatedScrollY;
	}

	rotate2d(&markerMoveX, &markerMoveY, -canvas.rotz);

	canvas.draggedMarker -> tileCoordinate.x += markerMoveX - canvas.dx;
	canvas.draggedMarker -> tileCoordinate.y += markerMoveY - canvas.dy;
	adjustTileCoordinateShifts(&canvas.draggedMarker -> tileCoordinate);
	toWorldCoordinate(&canvas.draggedMarker -> tileCoordinate, &canvas.draggedMarker -> geoCodeResult -> centroid);

	if (canvas.draggedMarker -> type == ROUTE_START || canvas.draggedMarker -> type == ROUTE_END) {
		if (canvas.route.query == NULL) {
			canvas.route.query = calloc(1, sizeof(RoutingQuery));
		}
		if (canvas.draggedMarker -> type == ROUTE_START) {
			canvas.route.query -> start = canvas.draggedMarker -> geoCodeResult -> centroid;
		} else {
			canvas.route.query -> end = canvas.draggedMarker -> geoCodeResult -> centroid;
		}

		canvas.routeMarkersChanged = TRUE;
	}
}

PopupMenuEntry* createPopupEntry(void* argument, void(*handlePressed)(void*), char* displayName) {
	PopupMenuEntry* entry = calloc(1, sizeof(PopupMenuEntry));
	entry -> argument = argument;
	entry -> displayName = displayName;
	entry -> handlePressed = handlePressed;
	return entry;
}

void directionsFromHere(void* worldCoordinatePtr) {
	WorldCoordinate* wc = (WorldCoordinate*) worldCoordinatePtr;
	if (canvas.route.query == NULL) {
		canvas.route.query = calloc(1, sizeof(RoutingQuery));
		canvas.route.query -> end = canvas.currentPos;
	}
	canvas.route.query -> start = *wc;
	canvas.routeMarkersChanged = TRUE;

	if (canvas.routeMarkers == NULL) {
		createRouteMarkers();
	} else {
		updateRouteMarkers(ROUTE_START);
	}

}

void directionsToHere(void* worldCoordinatePtr) {
	WorldCoordinate* wc = (WorldCoordinate*) worldCoordinatePtr;
	if (canvas.route.query == NULL) {
		canvas.route.query = calloc(1, sizeof(RoutingQuery));
		canvas.route.query -> start = canvas.currentPos;
	}
	canvas.route.query -> end = *wc;
	canvas.routeMarkersChanged = TRUE;

	if (canvas.routeMarkers == NULL) {
		createRouteMarkers();
	} else {
		updateRouteMarkers(ROUTE_END);
	}

}

void freePopupMenu() {
	if (canvas.popup.popupEntries != NULL) {
		g_list_free(canvas.popup.popupEntries);
		canvas.popup.popupEntries = NULL;
	}
}

void createSearchMarkerOnWorldCoordinate(void* wcPtr) {
	WorldCoordinate* wc = wcPtr;
	GeoCodeResult* gc = createGeoCodeResult(wc);
	ScreenMarker* marker = createMarker(gc, SEARCH_RESULT);
	canvas.markers = g_list_prepend(canvas.markers, marker);
}

void clearMap(void* unused) {
	clearBuildings();
	tileEngineFreeRoute();
	deallocateSearchMarkers();
	clearRouteMarkers();
}

void createPopupMenu() {
	freePopupMenu();
	TileCoordinate tc;
	setClickedTileCoordinate(&tc);
	WorldCoordinate* wc = calloc(1, sizeof(WorldCoordinate));
	toWorldCoordinate(&tc, wc);

	PopupMenuEntry* entry;

	entry = createPopupEntry(wc, &directionsFromHere, "Directions from here");
	canvas.popup.popupEntries = g_list_append(canvas.popup.popupEntries, entry);

	entry = createPopupEntry(wc, &directionsToHere, "Directions to here");
	canvas.popup.popupEntries = g_list_append(canvas.popup.popupEntries, entry);

	entry = createPopupEntry(wc, &createSearchMarkerOnWorldCoordinate, "Create marker here");
	canvas.popup.popupEntries = g_list_append(canvas.popup.popupEntries, entry);

	entry = createPopupEntry(wc, &clearMap, "Clear map");
	canvas.popup.popupEntries = g_list_append(canvas.popup.popupEntries, entry);

	canvas.popup.changed = TRUE;
	canvas.popup.currentClickCreatedPopup = TRUE;
}

void handleMouseHoldOnMap() {
	TileCoordinate tc;
	if (fabs(canvas.dx) > 0.1 || fabs(canvas.dy) > 0.1) {
		return;
	}

	if (canvas.mouseHoldInPlace < 0) {
		setClickedTileCoordinate(&tc);
		toWorldCoordinate(&tc, &canvas.mouseHoldPositionWc);
	}

	toTileCoordinate(&canvas.mouseHoldPositionWc, &tc, canvas.zoom);
	toScreenCoordinate(&tc, &canvas.mouseHoldPositionSc);

	canvas.mouseHoldInPlace = nowMillis - mouse.pressed;

	if (canvas.popup.popupEntries == NULL && canvas.mouseHoldInPlace > options.mouseHoldToPopupTime) {
		createPopupMenu();
	}
}

void handleMouseHold() {
	ScreenMarker* marker = findMarkerUnderMouse();
	int mouseHoldInPlace = FALSE;

	if (mouse.draggedDistanceX < 15 && mouse.draggedDistanceY < 15) {
		mouseHoldInPlace = TRUE;
	}

	if (mouseHoldInPlace) {
		if (canvas.draggedMarker == NULL && marker != NULL) {
			marker -> raised = 1;
			canvas.draggedMarker = marker;
		}
	}

	if (canvas.draggedMarker != NULL) {
		handleDraggedMarker();
	} else if (mouseHoldInPlace) {
		handleMouseHoldOnMap();
	} else {
		canvas.mouseHoldInPlace = -1;
	}
}

GLfloat getPopupEntryHeight();
void updateClickedPopupEntry() {
	if (canvas.popup.popupEntries == NULL) {
		canvas.popup.clickedPopupEntry = NULL;
		return;
	}
	int x, y;

	TileCoordinate tc;
	Point clickedPoint;
	setClickedTileCoordinate(&tc);
	toScreenCoordinate(&tc, &clickedPoint);

	x = clickedPoint.x - canvas.popup.x;
	y = clickedPoint.y - canvas.popup.y;
	if (x < 0 || y < 0 || x > canvas.popup.width || y >= canvas.popup.height) {
		canvas.popup.clickedPopupEntry = NULL;
		return;
	}
	int idx = y / getPopupEntryHeight();
	int length = g_list_length(canvas.popup.popupEntries);
	if (idx < 0 || idx >= length) {
		fprintf(stderr, "Internal error: clicked popup index out of range, popupSize = %d, index = %d\n", length, idx);
		canvas.popup.clickedPopupEntry = NULL;
		return;
	}
	canvas.popup.clickedPopupEntry = g_list_nth(canvas.popup.popupEntries, idx) -> data;
}

void tileEngineHandleMouseDown() {
	if (canvas.popup.popupEntries != NULL) {
		if (canvas.popup.animationFrame >= 30) {
			if (canvas.popup.clickedPopupEntry == NULL) {
				freePopupMenu();
			}
		}
	}
}

void tileEngineProcessMouse(int uiElemPressed) {
	if (uiElemPressed == TRUE) {
		return;
	}

	updateClickedPopupEntry();
	if (mouse.button != 0) {
		if (mouse.oldButton == 0) {
			tileEngineHandleMouseDown();
		}
		canvas.followingMypos = 0;
		canvas.routeFlybyMode = 0;
		canvas.attractionToPoint = 0;
		if (canvas.viewMode == VIEW_3D || canvas.rotation2d == 1) {
			canvas.attractedToRotZ = 0;
		}
		if (mouse.oldButton != 0 && nowMillis - mouse.pressed > 200) {
			handleMouseHold();
		}
	} else {
		if (mouse.oldButton != 0 && nowMillis - mouse.pressed < 200) {
			handleMouseClick();
		}
		if (mouse.oldButton != 0) {
			handleMouseUp();
		}
	}

	if ((canvas.viewMode == VIEW_3D || canvas.rotation2d == 1) && canvas.draggedMarker == NULL) {
		if (options.orientation == PORTRAIT && mouse.x < 150) {
			canvas.drotz -= mouse.ydelta / 18.0;
			return;
		}
		if (options.orientation == LANDSCAPE && mouse.y < 150) {
			canvas.drotz += mouse.xdelta / 25.0;
			return;
		}
	}

	if (mouse.button && canvas.draggedMarker == NULL && canvas.popup.popupEntries == NULL) {
		GLfloat dx = mouse.ydelta * sin(canvas.rotz * M_PI / 180.0) + mouse.xdelta * sin((canvas.rotz + 90) * M_PI / 180.0);
		GLfloat dy = mouse.ydelta * cos(canvas.rotz * M_PI / 180.0) + mouse.xdelta * cos((canvas.rotz + 90) * M_PI / 180.0);
		canvas.dx += dx / 10.0 / canvas.scale;
		canvas.dy += dy / 10.0 / canvas.scale;
		if (nowMillis - mouse.moved > 50) {
			canvas.friction = .2;
		} else {
			canvas.friction = .97;
		}
	}
}
void tileEngineChangeOrientation(Orientation orientation);

void tileEngineProcessAccelerometer() {
	if (options.accelerometerEnabled == 1) {

		if (canvas.viewMode == VIEW_2D) {
			if (options.orientation == LANDSCAPE) {
				if (abs(accelerometer.x) > 70) {
					canvas.dx += accelerometer.x / 1000.0 * sin((canvas.rotz + 90) * M_PI / 180.0);
					canvas.dy += accelerometer.x / 1000.0 * cos((canvas.rotz + 90) * M_PI / 180.0);
				}
				if (abs(accelerometer.y) > 70) {
					canvas.dx += accelerometer.y / 1000.0 * sin(canvas.rotz * M_PI / 180.0);
					canvas.dy += accelerometer.y / 1000.0 * cos(canvas.rotz * M_PI / 180.0);
				}
			} else {
				if (abs(accelerometer.y) > 70) {
					canvas.dx += accelerometer.y / 1000.0 * sin(canvas.rotz * M_PI / 180.0);
					canvas.dy += accelerometer.y / 1000.0 * cos(canvas.rotz * M_PI / 180.0);
				}
				if (abs(accelerometer.x) > 70) {
					canvas.dx += accelerometer.x / 1000.0 * sin((canvas.rotz + 90) * M_PI / 180.0);
					canvas.dy += accelerometer.x / 1000.0 * cos((canvas.rotz + 90) * M_PI / 180.0);
				}
			}
		} else {
			if (options.orientation == LANDSCAPE) {
				if (abs(accelerometer.y) > 70) {
					canvas.dx += accelerometer.y / 1000.0 * sin(canvas.rotz * M_PI / 180.0);
					canvas.dy += accelerometer.y / 1000.0 * cos(canvas.rotz * M_PI / 180.0);
				}
			} else {
				if (abs(accelerometer.x) > 70) {
					canvas.dx += accelerometer.x / 1000.0 * sin((canvas.rotz + 90) * M_PI / 180.0);
					canvas.dy += accelerometer.x / 1000.0 * cos((canvas.rotz + 90) * M_PI / 180.0);
				}
			}
			if (options.orientation == LANDSCAPE) {
				canvas.drotz += accelerometer.x / 2000.0;
				if (abs(accelerometer.x) > 70) {
					canvas.attractedToRotZ = 0;
				}
			} else {
				canvas.drotz -= accelerometer.y / 2000.0;
				if (abs(accelerometer.y) > 70) {
					canvas.attractedToRotZ = 0;
				}
			}
		}
	}
	if (accelerometer.y + accelerometer.calibrateY < -850) {
		tileEngineChangeOrientation(LANDSCAPE);
	}
	if (accelerometer.x + accelerometer.calibrateX < -850) {
		tileEngineChangeOrientation(PORTRAIT);
	}
}

void updateCanvasCenterWorldCoordinate() {
	TileCoordinate tc;
	canvasCenterToTileCoordinate(&tc);

	int horizontalShift = roundDown(tc.x / (double) TILE_SIZE) - getCanvasShiftX();
	int verticalShift = roundDown(tc.y / (double) TILE_SIZE) - getCanvasShiftY();

	tc.x -= horizontalShift * TILE_SIZE;
	tc.y -= verticalShift * TILE_SIZE;

	tc.tilex += horizontalShift;
	tc.tiley += verticalShift;

	toWorldCoordinate(&tc, &canvas.center);
}

void tileEngineZoomKnotPressed() {
	canvas.zoomBarLastDragged = nowMillis;
	canvas.zoomBarVisibleMilis = nowMillis + 10000;
}

// Calculates zoom level from zoom knot position.
int getKnotDestZoom() {
	GLfloat screenSize = canvas.orientationTransitionLinear < 80 ? SCREEN_WIDTH : SCREEN_HEIGHT;
	int zoomLevelCount = canvas.provider -> maxZoom - canvas.provider -> minZoom + 1;
	return (canvas.orientationTransitionLinear < 80 ? zoomKnot -> x - 82 : (SCREEN_HEIGHT - zoomKnot -> y - 82 - 64)) / ((screenSize - 232) / zoomLevelCount) + canvas.provider -> minZoom;
}

void tileEngineZoomKnotDragged() {
	canvas.zoomBarActive = TRUE;
	canvas.zoomBarLastDragged = nowMillis;
	canvas.zoomBarVisibleMilis = nowMillis + 7000;
	canvas.zoomKnotPosition += canvas.orientationTransitionLinear < 80 ? mouse.x - mouse.oldx : mouse.y - mouse.oldy;
	if (canvas.zoomKnotPosition < 100 - 18) {
		canvas.zoomKnotPosition = 100 - 18;
	}
	int screenSize = canvas.orientationTransitionLinear < 80 ? SCREEN_WIDTH : SCREEN_HEIGHT;
	if (canvas.zoomKnotPosition > screenSize - 100 - 32 - 14) {
		canvas.zoomKnotPosition = screenSize - 100 - 32 - 14;
	}
	if (canvas.zoom != getKnotDestZoom()) {
		canvas.destScale = pow(2.0, getKnotDestZoom() - canvas.zoom);
		canvas.zoomingSpeed = 20;
	}
}

void tileEngineToggleSearchBar() {
	canvas.searchBarActive = 1 - canvas.searchBarActive;
	//		if (canvas.searchBarActive == 0) {
	//			SDL_StopTextInput();
	//		} else {
	//			SDL_Rect rect;
	//			rect.x = 100;
	//			rect.y = 100;
	//			rect.w = 500;
	//			rect.h = 100;
	//			SDL_SetTextInputRect(&rect);
	//			SDL_StartTextInput();
	//		}
}

double prevDistance = 0;
void tileEngineUpdateCoordinates() {
	//int zoomChange = (int) round(log2((double) canvas.scale));
	int zoomChange = 0;

	canvas.x += canvas.dx;
	canvas.y += canvas.dy;

	canvas.dx *= canvas.friction;
	canvas.dy *= canvas.friction;

	if (canvas.followingMypos) {
#ifdef N900
		if (device -> fix -> fields | LOCATION_GPS_DEVICE_LATLONG_SET) {
			canvas.attraction.latitude = device -> fix -> latitude;
			canvas.attraction.longitude = device -> fix -> longitude;
			canvas.attractionToPoint = 1;
		}
		if (canvas.viewMode == VIEW_3D || canvas.rotation2d == 1) {
			if ((device -> fix -> fields | LOCATION_GPS_DEVICE_TRACK_SET) && device -> satellites_in_use > 0 && device -> fix -> speed > 3) {
				if (options.orientation == LANDSCAPE) {
					canvas.destRotz = -device -> fix -> track;
				} else {
					canvas.destRotz = 270 - device -> fix -> track;
				}
				canvas.attractedToRotZ = 1;
			}
		}
#else
		canvas.attraction = canvas.currentPos;
		canvas.attractionToPoint = 1;
#endif
	} else if (canvas.routeFlybyMode == 1) {
		if (canvas.route.optimizedRoute.lineStripTileCoordinates == NULL) {
			canvas.routeFlybyMode = 0;
		} else {
			TileCoordinate tc;
			WorldCoordinate wc;
			tc = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, 0);
			toWorldCoordinate(&tc, &wc);
			canvas.attraction = wc;
			canvas.attractionToPoint = 1;

			if (getDistance(&canvas.attraction, &canvas.center) < 1.0) {
				canvas.routeFlybyMode = 2;
				canvas.routeFlybyLineIdx = 0;
				canvas.dx = 0;
				canvas.dy = 0;
			}
		}
	} else if (canvas.routeFlybyMode == 2) {
		if (canvas.route.optimizedRoute.lineStripTileCoordinates == NULL) {
			canvas.routeFlybyMode = 0;
		} else {
			TileCoordinate nextRoutePointTc;

			double distance = getDistance(&canvas.center, &canvas.center);

			if ((prevDistance >= 0 && distance >= prevDistance) || canvas.routeFlybyLineIdx == 0) {
				if (canvas.routeFlybyLineIdx < canvas.route.optimizedRoute.lineStripTileCoordinates -> len - 1) {
					if (canvas.routeFlybyLineIdx <= canvas.navigationStatus.currentLineIdx + 1) {
						TileCoordinate prevRoutePointTc = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates,
													TileCoordinate, canvas.routeFlybyLineIdx);
						canvas.routeFlybyLineIdx++;
						nextRoutePointTc = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates,
								TileCoordinate, canvas.routeFlybyLineIdx);

						if (canvas.viewMode == VIEW_3D || canvas.rotation2d) {

							Point prevSc, nextSc;
							toScreenCoordinate(&prevRoutePointTc, &prevSc);
							toScreenCoordinate(&nextRoutePointTc, &nextSc);
							Point e; // east
							e.x = nextSc.x - prevSc.x;
							e.y = nextSc.y - prevSc.y;

							if (e.x != 0.0) {
								Point zero;
								zero.x = 0.0;
								zero.y = 0.0;
								double length = pointDistance(&zero, &e);
								e.x /= length;
								e.y /= length;
								double theta = atan2(e.y, e.x);
								canvas.destRotz = -theta / M_PI * 180.0;
								if (options.orientation == LANDSCAPE) {
									canvas.destRotz -= 90.0;
									if (canvas.destRotz < -180) {
										canvas.destRotz += 360;
									}
								} else {
									canvas.destRotz += 180.0;
									if (canvas.destRotz > 180) {
										canvas.destRotz -= 360;
									}
								}
								canvas.attractedToRotZ = 1;
							}
						}
						toWorldCoordinate(&nextRoutePointTc, &canvas.attraction);
						prevDistance = -1;
					}
				} else {
					canvas.routeFlybyMode = 0;
//					canvas.attractionToPoint = 0;
//					canvas.dx = 0;
//					canvas.dy = 0;
				}
			} else {
				prevDistance = distance;
			}
		}
	}

	if (canvas.attractionToPoint) {
		Point distance, tmp;

		TileCoordinate tc;
		toTileCoordinate(&canvas.attraction, &tc, canvas.zoom);
		toScreenCoordinate(&tc, &distance);

		distance.x = (SCREEN_WIDTH / 2.0 - (canvas.x - TILE_SIZE + distance.x));
		distance.y = (SCREEN_HEIGHT / 2.0 - (canvas.y - TILE_SIZE + distance.y));

		tmp = distance;

		if (canvas.attractionZooming == 0 && (fabs(tmp.x) > 1000 || fabs(tmp.y) > 1000)) {
			canvas.attractionZooming = -1;
			canvas.attractionIntermediateZoom = canvas.zoom;

			while (fabs(tmp.x) > TILE_SIZE || fabs(tmp.y) > TILE_SIZE || (canvas.attractionDestZoom != -1 && canvas.attractionIntermediateZoom > canvas.attractionDestZoom)) {
				if (canvas.destScale == 0.0) {
					canvas.destScale = 0.5;
				} else {
					canvas.destScale /= 2.0;
				}
				tmp.x /= 2.0;
				tmp.y /= 2.0;
				canvas.attractionIntermediateZoom--;
			}

			if (canvas.attractionDestZoom == -1) {
				canvas.attractionDestZoom = canvas.zoom;
			}
			canvas.zoomingSpeed = 25;
		}

		if ((canvas.attractionZooming == -1 && fabs(distance.x) < 30 && fabs(distance.y) < 30 && (canvas.destScale == 0.0 || canvas.destScale >= 0.5) && canvas.scale < 1.02)
				|| (canvas.attractionZooming == 0 && canvas.attractionDestZoom != -1)) {
			if (canvas.destScale == 0.0) {
				canvas.destScale = 1.0;
			}
			if (canvas.attractionZooming == 0 && canvas.attractionDestZoom != -1) {
				canvas.destScale *= pow(2.0, canvas.attractionDestZoom - canvas.zoom);
			} else {
				canvas.destScale *= pow(2.0, canvas.attractionDestZoom - canvas.attractionIntermediateZoom);
			}
			canvas.attractionZooming = 1;
			canvas.zoomingSpeed = 25;
		}

		canvas.friction = .99;

		if (canvas.routeFlybyMode == 2) {
			Point p;
			p.x = 0.0;
			p.y = 0.0;
			double dist = pointDistance(&p, &tmp);
			canvas.dx = tmp.x * 3.0 / dist / canvas.scale;
			canvas.dy = tmp.y * 3.0 / dist / canvas.scale;
		} else {
			canvas.dx = (tmp.x / 20.0) / canvas.scale;
			canvas.dy = (tmp.y / 20.0) / canvas.scale;
		}

		if (canvas.destScale == 0.0 && canvas.attractionZooming == 1) {
			canvas.attractionZooming = 0;
			canvas.attractionDestZoom = -1;
			canvas.attractionIntermediateZoom = -1;
		}

		if (canvas.attractionZooming != 0) {
			GLfloat speed;
			if (canvas.attractionDestZoom - canvas.attractionIntermediateZoom != 0) {
				speed = cos((canvas.zoom - canvas.attractionIntermediateZoom) / (canvas.attractionDestZoom - canvas.attractionIntermediateZoom) * M_PI / 2.0) + 0.2;
			} else {
				speed = cos((canvas.zoom - canvas.attractionIntermediateZoom) / (canvas.attractionDestZoom) * M_PI / 2.0) + 0.2;
			}
			GLfloat slowDown = pow(canvas.attractionZooming == 1 ? .90 : 2.7, canvas.zoom - canvas.attractionIntermediateZoom + 0.5);

			canvas.dx *= speed / slowDown;
			canvas.dy *= speed / slowDown;
		}
	} else {
		canvas.attractionZooming = 0;
		canvas.attractionIntermediateZoom = -1;
		canvas.attractionDestZoom = -1;
	}

	if (canvas.destScale != 0.0) {
		canvas.oldScale = canvas.scale;

		GLfloat tmp;
		if (canvas.destScale > canvas.scale) {
			tmp = (canvas.scale - canvas.destScale) / canvas.zoomingSpeed;
			if (tmp < -0.045 * canvas.scale) {
				tmp = -0.045 * canvas.scale;
			}
			canvas.scale -= tmp /*/ canvas.destScale*/;
		} else {
			tmp = (canvas.destScale - canvas.scale) / canvas.zoomingSpeed;
			if (tmp < -0.05 * canvas.scale) {
				tmp = -0.05 * canvas.scale;
			}
			canvas.scale += tmp;
		}

		if (fabs(tmp) < 0.0001) {
			canvas.scale = canvas.destScale;
			canvas.destScale = 0.0;
		}
	}
	if (canvas.scale < 1.0) {
		zoomChange = -1;
	}

	if (canvas.scale >= 2.0) {
		zoomChange = 1;
	}

	if ((canvas.zoom + zoomChange) < canvas.provider -> minZoom || (canvas.zoom + zoomChange) > canvas.provider -> maxZoom) {
		zoomChange = 0;
	}

	if (zoomChange == 0) {
		int horizontalShift = roundDown(canvas.x / TILE_SIZE);
		int verticalShift = roundDown(canvas.y / TILE_SIZE);

		if (horizontalShift != 0 || verticalShift != 0) {
			canvas.x -= horizontalShift * TILE_SIZE;
			canvas.y -= verticalShift * TILE_SIZE;

			canvas.tilex -= horizontalShift;
			canvas.tiley -= verticalShift;
			updateTiles(horizontalShift, verticalShift);
		}
	}

	updateCanvasCenterWorldCoordinate();

	if (zoomChange != 0 /* && nowMillis - lastZoomChangeMilis > 500 */) {
		canvas.oldZoom = canvas.zoom;
		canvas.zoom += zoomChange;
		handleZoomChange();

		if (zoomChange == 1) {
			canvas.scale /= 2.0;
			if (canvas.destScale != 0.0) {
				canvas.destScale /= 2.0;
			}
		} else if (zoomChange == -1) {
			canvas.scale *= 2.0;
			if (canvas.destScale != 0.0) {
				canvas.destScale *= 2.0;
			}
		}
	}

	canvas.rotx += (canvas.destRotx - canvas.rotx) * 0.03;
	canvas.roty += (canvas.destRoty - canvas.roty) * 0.03;
	canvas.orientationTransition += ((GLfloat) options.orientation - canvas.orientationTransition) * 0.03;
	if (options.orientation == LANDSCAPE) {
		if (canvas.orientationTransitionLinear > 0) {
			canvas.orientationTransitionLinear--;
		}
	} else {
		if (canvas.orientationTransitionLinear < 160) {
			canvas.orientationTransitionLinear++;
		}
	}
	if (canvas.attractedToRotZ) {
		double dist = canvas.destRotz - canvas.rotz, dist2 = 180.0;
		if (canvas.rotz < 0.0 && canvas.destRotz > 0.0) {
			dist2 = -(180.0 + canvas.rotz) - (180.0 - canvas.destRotz);
		} else if (canvas.rotz > 0.0 && canvas.destRotz < 0.0) {
			dist2 = 180 - canvas.rotz + (180.0 + canvas.destRotz);
		}
		if (fabs(dist) < fabs(dist2)) {
			canvas.rotz += dist * 0.03;
		} else {
			canvas.rotz += dist2 * 0.03;
		}
	} else {
		canvas.rotz += canvas.drotz;
		canvas.drotz *= 0.9;
	}
	if (canvas.rotz > 180) {
		canvas.rotz -= 360;
	}
	if (canvas.rotz < -180) {
		canvas.rotz += 360;
	}
	if (canvas.orientationTransition > 0.999) {
		canvas.orientationTransition = 1.0;
	} else if (canvas.orientationTransition < 0.001) {
		canvas.orientationTransition = 0.0;
	}
}

void activateZoomBar() {
	canvas.zoomBarActive = TRUE;
	canvas.zoomBarVisibleMilis = nowMillis + 3000;
	int zoomLevelCount = canvas.provider -> maxZoom - canvas.provider -> minZoom + 2;
	GLfloat knotZoomLevel = canvas.zoom - canvas.provider -> minZoom;
	if (canvas.destScale != 0.0) {
		knotZoomLevel += round(log2(canvas.destScale));
	}
	if (canvas.orientationTransitionLinear < 80) {
		canvas.zoomKnotPosition = knotZoomLevel * (SCREEN_WIDTH - 228) / zoomLevelCount + 114;
	} else {
		canvas.zoomKnotPosition = -(knotZoomLevel * (SCREEN_HEIGHT - 232) / zoomLevelCount) + SCREEN_HEIGHT - 114 - 32 - 14;
	}
}

long zoomInLastMilis = 0;
void tileEngineZoomIn() {
	if (SDL_GetModState() & KMOD_SHIFT) {
		canvas.oldScale = canvas.scale;
		canvas.scale *= 1.005;
	} else {
		if (nowMillis - zoomInLastMilis > 100) {
			if (canvas.destScale == 0.0) {
				if (canvas.scale <= 0.5) {
					canvas.destScale = 1.0;
				} else {
					canvas.destScale = 2.0;
				}
			} else if (canvas.scale < 2.0 && canvas.destScale <= 2.0 && canvas.zoom < canvas.provider -> maxZoom) {
				canvas.destScale *= 2.0;
			}
			canvas.zoomingSpeed = 25;
			canvas.attractionZooming = 0;

			zoomInLastMilis = nowMillis;
			if (mouse.oldButton == 0 && pressedUiElem == zoomIn) {
				zoomInLastMilis += 400;
			}
			activateZoomBar();
		}
	}
}
void tileEngineViewMode2D();

long zoomOutLastMilis = 0;
void tileEngineZoomOut() {
	if (SDL_GetModState() & KMOD_SHIFT) {
		canvas.oldScale = canvas.scale;
		canvas.scale /= 1.005;
	} else {
		if (nowMillis - zoomOutLastMilis > 100) {
			if (canvas.destScale == 0.0) {
				if (canvas.scale >= 2.0) {
					canvas.destScale = 1.0;
				} else {
					canvas.destScale = 0.5;
				}
			} else if (canvas.scale > 0.25 && canvas.destScale >= 0.25 && canvas.zoom > canvas.provider -> minZoom) {
				canvas.destScale /= 2.0;
			}
			canvas.zoomingSpeed = 25;
			canvas.attractionZooming = 0;
			zoomOutLastMilis = nowMillis;
			if (mouse.oldButton == 0 && pressedUiElem == zoomOut) {
				zoomOutLastMilis += 400;
			}
			activateZoomBar();
		}
	}
}

void tileEngineGotomypos() {
	if (mouse.oldButton == 0) {
		canvas.followingMypos = 1 - canvas.followingMypos;
		canvas.attractionZooming = 0;
		canvas.routeFlybyMode = 0;
		if (canvas.followingMypos) {
			updateCanvasCenterWorldCoordinate();
			canvas.previousCenter.latitude = canvas.center.latitude;
			canvas.previousCenter.longitude = canvas.center.longitude;
		}
	}
}

void tileEngineViewMode2D() {
	view2d -> status = UI_HIDDEN;
	view3d -> status = UI_SHOWN;
	canvas.viewMode = VIEW_2D;
	if (options.orientation == LANDSCAPE) {
		canvas.destRotx = 0.0;
		canvas.destRoty = 0.0;
	} else {
		canvas.destRotx = 0.0;
		canvas.destRoty = 0.0;
	}
	if (canvas.rotation2d == 0) {
		if (options.orientation == LANDSCAPE) {
			canvas.destRotz = 0.0;
		} else {
			canvas.destRotz = -90.0;
		}
		canvas.attractedToRotZ = 1;
	}
}

void tileEngineViewMode3D() {
	view2d -> status = UI_SHOWN;
	view3d -> status = UI_HIDDEN;
	canvas.viewMode = VIEW_3D;
	if (options.orientation == LANDSCAPE) {
		canvas.destRotx = LANDSCAPE_ROTATION_X;
		canvas.destRoty = 0.0;
	} else {
		canvas.destRotx = 0.0;
		canvas.destRoty = -PORTRAIT_ROTATION_Y;
	}
}

int zoomBarInitialized = FALSE;

int isZoomBarInitialized() {
	return zoomBarInitialized;
}

void setZoomBarInitialized(int value) {
	zoomBarInitialized = value;
}

void addSearchMarkerAtCanvasCenter() {
	GeoCodeResult* r = createGeoCodeResult(&canvas.center);
	ScreenMarker* marker = createMarker(r, SEARCH_RESULT);
	canvas.markers = g_list_prepend(canvas.markers, marker);
}

void tmpQueryRoute() {
	RoutingQuery* q = calloc(1, sizeof(RoutingQuery));
	if (q != NULL) {
		q -> start.latitude = canvas.currentPos.latitude;
		q -> start.longitude = canvas.currentPos.longitude;
		q -> end.latitude = canvas.center.latitude;
		q -> end.longitude = canvas.center.longitude;
		processNewRoutingQuery(q);
	}
}

void tileEngineChangeOrientation(Orientation orientation) {
	if (orientation != options.orientation) {
		if (canvas.viewMode == VIEW_2D && canvas.rotation2d == 0) {
			if (orientation == LANDSCAPE) {
				canvas.destRotz = 0.0;
			} else {
				canvas.destRotz = -90.0;
			}
		} else {
			if (orientation == LANDSCAPE) {
				canvas.destRotz = canvas.rotz + 90;
				if (canvas.destRotz > 180) {
					canvas.destRotz -= 360;
				}
			} else {
				canvas.destRotz = canvas.rotz - 90;
				if (canvas.destRotz < -180) {
					canvas.destRotz += 360;
				}
			}
		}
		options.orientation = orientation;
		if (canvas.viewMode == VIEW_2D) {
			tileEngineViewMode2D();
		} else {
			tileEngineViewMode3D();
		}
		canvas.attractedToRotZ = 1;
		canvas.zoomBarActive = 0;
	}
}

void tileEngineToggle2dRotation() {
	if (canvas.viewMode == VIEW_2D) {
		canvas.rotation2d = 1 - canvas.rotation2d;
		if (canvas.rotation2d == 0) {
			canvas.attractedToRotZ = 1;
			if (options.orientation == LANDSCAPE) {
				canvas.destRotz = 0.0;
			} else {
				canvas.destRotz = -90.0;
			}
		} else {
			canvas.attractedToRotZ = 0;
		}
	} else {
		if (options.orientation == LANDSCAPE) {
			canvas.destRotz = 0.0;
		} else {
			canvas.destRotz = -90.0;
		}
		canvas.attractedToRotZ = 1;
	}
}

void shutdownTileEngine() {
	int i;
	for (i = 0; i < DOWNLOADING_THREADS; i++) {
		//        fprintf(stderr, "waiting for download thread %d to finish...\n", SDL_GetThreadID(downloadThreads[i]));
		//SDL_WaitThread(downloadThreads[i], NULL);
	}
}

void tileEngine_computeBoundingTrapezoid() {
	GLfloat posx, posy, posz;

	// tile plane z / (far plane z - near plane z)
	GLfloat z1 = 350.0 / (600.0 - 50.0);
	GLfloat z = z1 - canvas.roty / 15.0;
	if (z > 1.0) {
		z = 1.0;
	}

	gluUnProject(0, 0, z, &posx, &posy, &posz);
	canvas.boundingTrapezoid[0].tilex = canvas.tilex + getCanvasShiftX();
	canvas.boundingTrapezoid[0].tiley = canvas.tiley + getCanvasShiftY();
	canvas.boundingTrapezoid[0].x = posx;
	canvas.boundingTrapezoid[0].y = posy;
	canvas.boundingTrapezoid[0].zoom = canvas.zoom;

	z = z1;

	gluUnProject(SCREEN_WIDTH - 1, 0, z, &posx, &posy, &posz);

	canvas.boundingTrapezoid[1].tilex = canvas.tilex + getCanvasShiftX();
	canvas.boundingTrapezoid[1].tiley = canvas.tiley + getCanvasShiftY();
	canvas.boundingTrapezoid[1].x = posx;
	canvas.boundingTrapezoid[1].y = posy;
	canvas.boundingTrapezoid[1].zoom = canvas.zoom;

	z = z1 + canvas.rotx / 40.0;
	if (z > 1.0) {
		z = 1.0;
	}

	gluUnProject(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, z, &posx, &posy, &posz);

	canvas.boundingTrapezoid[2].tilex = canvas.tilex + getCanvasShiftX();
	canvas.boundingTrapezoid[2].tiley = canvas.tiley + getCanvasShiftY();
	canvas.boundingTrapezoid[2].x = posx;
	canvas.boundingTrapezoid[2].y = posy;
	canvas.boundingTrapezoid[2].zoom = canvas.zoom;

	z = z1 + canvas.rotx / 40.0 - canvas.roty / 15.0;
	if (z > 1.0) {
		z = 1.0;
	}

	gluUnProject(0, SCREEN_HEIGHT - 1, z, &posx, &posy, &posz);
	canvas.boundingTrapezoid[3].tilex = canvas.tilex + getCanvasShiftX();
	canvas.boundingTrapezoid[3].tiley = canvas.tiley + getCanvasShiftY();
	canvas.boundingTrapezoid[3].x = posx;
	canvas.boundingTrapezoid[3].y = posy;
	canvas.boundingTrapezoid[3].zoom = canvas.zoom;
}

/*
 * Compute the side of point (x, y) in relation to line segment (x0, y0) - (x1, y1)
 * returns <0 when point is on right side of line segment
 * returns 0 when point is belongs to line
 * return >0 when point is on left side
 */
GLfloat computeSide(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat x, GLfloat y) {
	GLfloat result = (y1 - y) * (x1 - x0) - (x1 - x) * (y1 - y0);
	//    fprintf(stderr, "    %f %f %f %f %f %f, result = %f\n", x0, y0, x1, y1, x, y, result);
	return result;
}

int tileEngine_isTileCoordinateInBoundingTrapezoid(int tilex, int tiley) {
	int i;

	for (i = 0; i < 4; i++) {
		if (computeSide(canvas.boundingTrapezoid[i].tilex + canvas.boundingTrapezoid[i].x / (GLfloat) TILE_SIZE,
				canvas.boundingTrapezoid[i].tiley + canvas.boundingTrapezoid[i].y / (GLfloat) TILE_SIZE,
				canvas.boundingTrapezoid[(i + 1) % 4].tilex + canvas.boundingTrapezoid[(i + 1) % 4].x / (GLfloat) TILE_SIZE,
				canvas.boundingTrapezoid[(i + 1) % 4].tiley + canvas.boundingTrapezoid[(i + 1) % 4].y / (GLfloat) TILE_SIZE, tilex, tiley) < 0) {
			//            fprintf(stderr, "    return FALSE\n");
			return FALSE;
		}
	}
	//    fprintf(stderr, "    return TRUE\n");
	return TRUE;
}

void tileEngine_switchTileProvider() {
}

void tileEngine_computeTilesVisibility() {
	int i, j;

	// tiles share coordinates so it is more efficient to precalculate visibility for each unique coordinate.
	for (i = 0; i < TILES_X + 1; i++) {
		for (j = 0; j < TILES_Y + 1; j++) {
			tileCoordinateVisibility[i][j] = tileEngine_isTileCoordinateInBoundingTrapezoid(canvas.tilex + i, canvas.tiley + j);
		}
	}

	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			if (tiles[i][j][currentTilesIdx] != NULL) {
				if (tileCoordinateVisibility[i][j] || tileCoordinateVisibility[i + 1][j] || tileCoordinateVisibility[i][j + 1] || tileCoordinateVisibility[i + 1][j + 1]) {
					if (tiles[i][j][currentTilesIdx] -> state == STATE_GL_TEXTURE_NOT_CREATED && texturesCreatedThisFrame < MAX_TEXTURES_CREATED_PER_FRAME) {
						tiles[i][j][currentTilesIdx]->texture = createTexture(tiles[i][j][currentTilesIdx] -> pixels4444, TILE_SIZE, TILE_SIZE, TRUE);
						// TODO tmp
						//						if (syncLoadedTilesAppearance == TRUE) {
						//							tiles[i][j][currentTilesIdx] -> state = STATE_LOADED_SYNC_WAIT;
						//						} else {
						tiles[i][j][currentTilesIdx] -> state = STATE_LOADED;
						tiles[i][j][currentTilesIdx] ->stateChangeTime = 0;
						//						}
						texturesCreatedThisFrame++;
					}
					tiles[i][j][currentTilesIdx] -> visible = TRUE;
				} else {
					tiles[i][j][currentTilesIdx] -> visible = FALSE;
					if ((tiles[i][j][currentTilesIdx] -> state == STATE_LOADED || tiles[i][j][currentTilesIdx] -> state == STATE_LOADED_SYNC_WAIT) && (tiles[i][j][currentTilesIdx] -> texture != 0)) {
						GLuint texture = tiles[i][j][currentTilesIdx] -> texture;
						tiles[i][j][currentTilesIdx] -> texture = 0;
						tiles[i][j][currentTilesIdx] -> state = STATE_GL_TEXTURE_NOT_CREATED;
						deleteTexture(texture);
					}
				}
			}
		}
	}
}
