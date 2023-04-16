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

#ifndef MAIN_H_
#define MAIN_H_

#include <glib-2.0/glib.h>
#include <SDL/SDL.h>
#include <math.h>

#if defined(N900) || defined N950
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GL/gl.h>
#endif

#define M_PI 3.14159265358979323846
#define CLOUDGPS_VERION "0.5.5"

/** Equatorial earth radius in meters. */
#define EARTH_RADIUS 6378137

#ifdef N900
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
#else
#define SCREEN_WIDTH  854
#define SCREEN_HEIGHT 480
#endif
#define TILE_SIZE     256
#define PORTRAIT_ROTATION_Y 14
#define LANDSCAPE_ROTATION_X 14

// how many satellite bars are drawn in status bar
#define MAX_SATTELITES 12

// how many tiles horizontally and vertically
#ifdef N900
#define TILES_X 6
#define TILES_Y 6
#else
#define TILES_X 8
#define TILES_Y 8
#endif


#define MAX_TEXTURES_CREATED_PER_FRAME 1

typedef enum {
    LANDSCAPE = 0, PORTRAIT = 1
} Orientation;

typedef enum {
	NO_QUERY, QUERY_IN_PROGRESS, DOWNLOAD_ERROR, PARSE_ERROR, ZERO_RESULTS, RESULTS_READY, QUERY_THE_SAME
} BackgroundQueryStatus;

typedef struct {
	char* name;
	char* (*prepareUrl)(void* query);
	void (*parseResponse)(char* response);
} BackgroundQueryProvider;

extern GList* geocodingProviders;
extern GList* routingProviders;

typedef struct {
    double latitude, longitude;
} WorldCoordinate;

typedef struct {
    int tilex, tiley, zoom;
    double x, y;
} TileCoordinate;

typedef struct {
	GLfloat x, y;
} Point;

typedef struct {
    GLfloat r, g, b, a;
} Color;

typedef struct {
   const char* name;
   const char* description;
   const char* wikipedia;
   WorldCoordinate centroid;
   WorldCoordinate bounds[2];
} GeoCodeResult;

typedef enum {
	SEARCH_RESULT, FAVOURITE, ROUTE_START, ROUTE_WAYPOINT, ROUTE_END
} MarkerType;

typedef struct {
	MarkerType type;
	GeoCodeResult* geoCodeResult;
	TileCoordinate tileCoordinate;
	Point screenCoordinate;
	int visible, raised, recalculateCoords, poi;
	GLfloat rotz;
	int raiseLevel;
} ScreenMarker;

typedef struct {
    int mipmapToggle;
    int showGrid, showCoordinates, accelerometerEnabled;
    int showDebugTiles;
    int locallyLoadedFadeTime, downloadedFadeTime, zoomChangeFadeTime;
    int curlVerbose;
    int forceIPv4;
    int mouseHoldToPopupTime;
    int popupEntryMargin;
    int loadLastRoute;
    int offlineMode;

    // TODO use them to determine behaviour on tile rendering
    int useFading, useTileSync;

    Orientation orientation;
    char *userAgent;
    char *referer;
    BackgroundQueryProvider* geocodingProvider;
    BackgroundQueryProvider* routingProvider;
    int snapToRouteToleranceMeters;
    int outsideRouteToleranceMeters;
    int recalculateRouteAfterOutSideRouteSeconds;
} OptionsStruct;

/* Actually defined in main.c */
extern OptionsStruct options;

//TTF_Font *font = NULL;

typedef struct {
    char* name;
    int minZoom, maxZoom;
    char* filenameFormat;
    char* urlFormat;
    char* urlFormatType;
    char quadKeyFirstChar;
} TileProvider;

typedef enum {
    VIEW_2D, VIEW_3D
} ViewMode;

typedef enum {
    UI_HIDDEN = 0, UI_COMING = 1, UI_SHOWN = 2, UI_GOING = 3
} uiElemStatus;

typedef enum {
	NONE = 0, LINEAR = 1, CUBIC = 2
} AttractionMode;

typedef struct {
    GLuint name;
    int size;
} Texture;

typedef struct {
	int displayNameWidth;
	char* displayName;
    void (*handlePressed)(void*);
	void* argument;
} PopupMenuEntry;

typedef struct {
	int x, y, height, width, changed;
	int animationFrame;
	int currentClickCreatedPopup;
	GList* popupEntries;
	PopupMenuEntry* clickedPopupEntry;
} PopupMenu;

typedef struct {
    GLfloat portraitx, portraity;
    GLfloat landscapex, landscapey;
    GLfloat x, y;
    GLfloat destx, desty;
    long stateChangeMilis;
    uiElemStatus status;
    Texture texture;
    Texture mask;
    Color color;
    void (*handlePressed)();
    void (*handleDragged)();
    GLfloat * texCoords;
} UiElement;

typedef struct {
	GArray* polyLine;
	char* text;
	int metersFromStart;
	int position;
} RouteDirection;

typedef struct {
	TileCoordinate p1, p2;
	RouteDirection* direction;
} VisibleLine;

typedef struct {
	GList* visibleLines;
	GArray* lineStripTileCoordinates;
	int lineStripZoom;
} OptimizedRoute;

typedef struct {
	WorldCoordinate start;
	WorldCoordinate end;
	GList* waypoints;
} RoutingQuery;

typedef struct {
	RoutingQuery* query;
	int lengthMeters;
	GArray* directions;
	OptimizedRoute optimizedRoute;
	char statusMessage[500];
} Route;

typedef struct {
	long outOfTrackMillis;
	double snappedDistance;
	WorldCoordinate snapped;
	RouteDirection* currentDirection;
	double distanceToTarget;
	int roundedDistanceToNextDirection;
	int currentDirectionIdx;
	int currentLineIdx;
} NavigationStatus;

typedef struct {
    GLfloat x, y, dx, dy, scale, oldScale, destScale, rotx, roty, rotz, destRotx, destRoty, destRotz, drotz, friction;
    int attractedToRotZ;
    int tilex, tiley;
    int zoom, oldZoom, attractionIntermediateZoom, attractionDestZoom;
    int attractionZooming; // 0 - not zooming, 1 - zooming in, -1 - zooming out
    WorldCoordinate center;
    WorldCoordinate previousCenter; // before following mode turned on
    WorldCoordinate attraction; // position to which canvas is attracted (ie current position in following mode)
    WorldCoordinate currentPos;
    TileProvider* provider;
    ViewMode viewMode;
    GLfloat orientationTransition;
    int orientationTransitionLinear;
    int followingMypos;
    int attractionToPoint;
    int routeFlybyMode;
    int routeFlybyLineIdx;

    int rotation2d;
    int searchBarActive;
    int zoomBarActive;
    long zoomBarVisibleMilis;
    long zoomBarLastDragged;
    GLfloat zoomBarAlpha;
    GLfloat zoomKnotPosition;
    GLfloat zoomingSpeed;
    GLfloat searchBarY;
    GArray* buildings;
    GList* markers;
    int buildingsVerticesPrecomputed;
    GLfloat buildingsHeight;
    GLfloat buildingsTileX;
    GLfloat buildingsTileY;
    TileCoordinate boundingTrapezoid[4]; // points in clockwise order

    int arrowPos; // 0 -none, 1 - left, 2 - right, 3 - up, 4 - down
    GLfloat arrowPosX, arrowPosY;
    Route route;
    GList* routeMarkers;
    volatile int lastSearchId;
    ScreenMarker* draggedMarker;
    ScreenMarker* dragCrosshairMarker;
    int routeMarkersChanged;
    int busy;
    int mouseHoldInPlace;
    WorldCoordinate mouseHoldPositionWc;
    Point mouseHoldPositionSc;
    PopupMenu popup;

    NavigationStatus navigationStatus;
} Canvas;
extern Canvas canvas;

#define SEARCHBAR_MAX_CHARS 100

typedef struct {
    char input[SEARCHBAR_MAX_CHARS];
    int cursorScreen; // Cursor position on screen (in pixels)
    int cursorInput; // Cursor position in input string (in bytes)
    int selectionStart;
    int selectionCount;
    int historyIndex;
    char** history;
    long lastChange;
} SearchBar;
extern SearchBar searchBar;

typedef struct {
    int x, y, oldx, oldy, button, oldButton, xdelta, ydelta;
    int clickedx, clickedy;
    int draggedDistanceX, draggedDistanceY;
    long pressed, moved; // milliseconds of last pressed and moved events, -1 if mousebutton is not down
} Mouse;
extern Mouse mouse;

typedef struct {
    int x, y, z;
    int calibrateX, calibrateY, calibrateZ;
    int performCalibration; // if set to TRUE calibration will be performed on next accelerometer read.
} Accelerometer;
extern Accelerometer accelerometer;

#define MAX_BUF_SIZE 500
extern char strbuf[MAX_BUF_SIZE];
extern char strbuf2[MAX_BUF_SIZE];

extern int quit;
extern int forceGarbageCollection;
extern int currentTilesIdx;
extern volatile int syncLoadedTilesAppearance;
extern int texturesCreatedThisFrame;
extern long nowMillis;
extern GQueue* downloadQueue;
extern int tileCoordinateVisibility[TILES_X + 1][TILES_Y + 1];

#ifdef N900
#include <location/location-gps-device.h>
extern LocationGPSDevice *device;
#endif

#endif /* MAIN_H_ */
