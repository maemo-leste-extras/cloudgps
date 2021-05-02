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

#ifndef TILE_H_
#define TILE_H_

//#include <sys/stat.h>
//#include <unistd.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <libgen.h>
//#include <errno.h>
//#include <glib-2.0/glib.h>
//#include <sys/time.h>

#include <glib.h>
//#include <SDL/SDL.h>
#include "main.h"


#define ALLOC_DEBUG 0

typedef enum {
    STATE_GARBAGE = -1, STATE_EMPTY = 0, STATE_QUEUE_DELAYED = 1, STATE_QUEUED = 2, STATE_LOADING = 3, STATE_ERROR = 4,
    // All below are guaranteed to have pixels4444 available
    STATE_LOADED = 5,
    STATE_SCALED = 6,
    STATE_GL_TEXTURE_NOT_CREATED = 7,
    STATE_LOADED_SYNC_WAIT = 8
} TileState;

typedef struct {
    volatile int zoom;
    volatile int tilex, tiley;
    volatile GLuint texture;
    volatile TileState state;
    volatile long stateChangeTime;
    int transitionTime;
    long textureUsedTime;
    volatile GLubyte deleted;
    volatile GLubyte visible;
    GLubyte zoomHelper;
    GLubyte textureShared;

    GLushort * pixels4444;
    TileProvider* provider;
    //char *errorMessage;
    volatile void * oldTiles[4];
} t_tile;


#define MAX_ZOOM_HELPER_TILES 20


void processDeferredDeallocation();
int downloadThread(void* queue);
void ensureDirExists(char *filename);
int isOutsideCanvas(t_tile * tile);

extern t_tile* tiles[TILES_X][TILES_Y][2];
extern t_tile* zoomTiles[MAX_ZOOM_HELPER_TILES][2];
extern GQueue* allCreatedTiles;

#endif /* TILE_H_ */
