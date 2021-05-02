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
#include <sys/resource.h>

#include "tile.h"
#include "main.h"

t_tile* tiles[TILES_X][TILES_Y][2];
t_tile* zoomTiles[MAX_ZOOM_HELPER_TILES][2];
GQueue* allCreatedTiles = NULL;


GQueue* getAllTilesCreated() {
	if (allCreatedTiles == NULL) {
		allCreatedTiles = g_queue_new();
	}

	return allCreatedTiles;
}


char* getTileFileName(TileProvider *provider, t_tile *tile);
char* getTileUrl(TileProvider *provider, t_tile *tile);
int downloadAndSave(char *url, char *filename);
GLushort * convert8888to4444(SDL_Surface* surfaceRGBA, int isTile);

gpointer dequeue(GQueue* queue);
void deleteElems(GQueue * queue, gpointer data);

GQueue* garbageQueue = NULL;

int isOutsideCanvas(t_tile * tile) {
    if ((tile -> tilex < canvas.tilex) || (tile -> tiley < canvas.tiley) || (tile -> tilex >= (canvas.tilex + TILES_X)) || (tile -> tiley
            >= (canvas.tiley + TILES_Y)) || (tile -> zoom != canvas.zoom) || (tile -> provider != canvas.provider)) {
        return TRUE;
    }
    return FALSE;
}

void deferredDeallocation(t_tile * tile, void* unused) {
    if (tile -> state == STATE_LOADING) {
        return;
    }

    if (nowMillis - tile -> textureUsedTime < 5000) {
        return;
    }
    if ((isOutsideCanvas(tile) && !tile -> zoomHelper) || tile -> deleted) {
        g_queue_remove_all(garbageQueue, tile);
        g_queue_push_tail(garbageQueue, tile);
    }
}
void deallocateTile(t_tile *tile);
void processDeferredDeallocation() {
    int i, j, k;
    t_tile * tile;

//    long start = SDL_GetTicks();
    garbageQueue = g_queue_new();

    // Add all outside current view to garbageQueue.
    g_queue_foreach(allCreatedTiles, (GFunc) deferredDeallocation, NULL);

    // Remove reffered by currently visible from garbageQueue.

    for (i = 0; i < TILES_X; i++) {
        for (j = 0; j < TILES_Y; j++) {
            if (tiles[i][j][currentTilesIdx] != NULL && tiles[i][j][currentTilesIdx] -> visible == TRUE) {
                tile = tiles[i][j][currentTilesIdx];
                if (tile != NULL) {
                    if ((tile -> state != STATE_LOADED) || ((tile -> state == STATE_LOADED) && nowMillis - tile->stateChangeTime
                            < tile->transitionTime)) {
                        for (k = 0; k < 4; k++) {
							void* oldTile = (void*) tile -> oldTiles[k];
                            if (oldTile != NULL) {
                                g_queue_remove_all(garbageQueue, oldTile);
                            }
                        }
                    }
                }
            }
        }
    }


    // Deallocate all garbage.
//    fprintf(stderr, "garbage collection (%ld ms): %d of %d tiles\n", (SDL_GetTicks() - start), g_queue_get_length(garbageQueue), g_queue_get_length(allCreatedTiles));
    g_queue_foreach(garbageQueue, (GFunc) deallocateTile, NULL);
    g_queue_clear(garbageQueue);
    g_queue_free(garbageQueue);
}

void deallocateTile(t_tile *tile) {
    if (tile == NULL) {
        return;
    }
    if (tile -> state == STATE_LOADING) {
        tile -> deleted = TRUE;
        return;
    }
    if (ALLOC_DEBUG) {
        fprintf(stderr, "deallocateTile(%p)\n", tile);
    }
    deleteElems(downloadQueue, (gpointer) tile);
    deleteElems(allCreatedTiles, (gpointer) tile);

    GLuint texture = tile->texture;
    tile->texture = 0;
    if (texture != 0) {
        deleteTexture(texture);
    }
    if (ALLOC_DEBUG) {
        fprintf(stderr, "deallocateTile, free pixels4444 %p\n", tile -> pixels4444);
    }

    free(tile -> pixels4444);
    tile -> pixels4444 = NULL;

    //    if (tile -> errorMessage) {
    //        fprintf(stderr, "deallocateTile, free errorMessage %p\n", tile -> errorMessage);
    //        free(tile -> errorMessage);
    //        tile -> errorMessage = NULL;
    //    }
    deleteElems(downloadQueue, (gpointer) tile);
    if (ALLOC_DEBUG) {
        fprintf(stderr, "deallocateTile, free tile %p\n", tile);
    }
//    free(tile);
}

// todo: move to file.c

SDL_Surface* loadTextureFromFile(char *filename, t_tile* tile, SDL_Surface** mask);

int loadTileTexture(char* filename, t_tile* tile) {
    SDL_Surface* texture = loadTextureFromFile(filename, tile, NULL);

    if (texture) {
        SDL_Delay(0);
        tile -> pixels4444 = texture -> pixels; //convert8888to4444(texture, TRUE);
        //        fprintf(stderr, "loadTextureFromFile, tile= %p allocated pixels4444 %p\n", tile, tile -> pixels4444);
        tile -> state = STATE_GL_TEXTURE_NOT_CREATED;
        //SDL_FreeSurface(texture);
        return TRUE;
    }
    return FALSE;
}

int shouldAbortLoading(t_tile * tile) {
    if (tile -> deleted) {
        tile -> state = STATE_GARBAGE;
        return TRUE;
    }

    if (!tile -> zoomHelper && isOutsideCanvas(tile)) {
        //        fprintf(stderr, "tile load abort, tilex = %d, tiley = %d, canvasx = %d, canvasy = %d, tile.zoom = %d, canvas.zoom = %d\n",
        //                tile -> tilex, tile -> tiley, canvas.tilex, canvas.tiley, tile -> zoom, canvas.zoom);
        tile -> state = STATE_GARBAGE;
        return TRUE;
    }
    return FALSE;
}

int debugPreventLoadingTiles = 0;

extern SDL_sem* sem;

int downloadThread(void* busyPtr) {
    t_tile* tile = NULL;
    int priorityStatus = 0;
    priorityStatus = setpriority(PRIO_PROCESS, 0, 19);
    GQueue* queue = downloadQueue;

    int* busy = busyPtr;

//    fprintf(stderr, "thread id: %p, setting priority: %d\n", (void*) SDL_ThreadID(), priorityStatus);
    while (!quit) {
    	*busy = FALSE;
        tile = (t_tile*) dequeue(queue);
        while (tile == NULL) {
            //            fprintf(stderr, "queue empty, waiting, thread id = %d\n", SDL_ThreadID());
            if (quit) {
                return 0;
            }
            if (SDL_SemWait(sem) == -1) {
                fprintf(stderr, "SemWait failed in downloadThread\n");
                exit(1);
            }
            SDL_Delay(20);
            tile = (t_tile*) dequeue(queue);
        }
    	*busy = TRUE;
        int mapSize = 1 << tile -> zoom;

        if (tile -> tilex < 0 || tile -> tiley < 0 || tile -> tilex >= mapSize || tile -> tiley >= mapSize || debugPreventLoadingTiles) {
            //            fprintf(stderr, "tile outside map\n");
            tile -> state = STATE_EMPTY;
            continue;
        }

//        if (tile -> oldTiles[0] != NULL) {
//        	t_tile* oldTile = (t_tile*) tile -> oldTiles[0];
//        	if (!isOutsideCanvas(oldTile)) {
//        		tile -> texture = oldTile -> texture;
//        		tile -> pixels4444 = oldTile -> pixels4444;
//
//        		oldTile -> texture = 0;
//        		oldTile -> pixels4444 = NULL;
//        		oldTile -> deleted = TRUE;
//        		oldTile -> state = STATE_GARBAGE;
//
//        		tile -> transitionTime = 0;
//        		tile -> state = STATE_LOADED;
//                        fprintf(stderr, "tile not necessary, tilex = %d, tiley = %d, canvasx = %d, canvasy = %d, tile.zoom = %d, canvas.zoom = %d\n",
//                                tile -> tilex, tile -> tiley, canvas.tilex, canvas.tiley, tile -> zoom, canvas.zoom);
//        		continue;
//        	}
//        }

        if (shouldAbortLoading(tile)) {
            continue;
        }

        tile -> state = STATE_LOADING;

        //        fprintf(stderr, "load tile start, thread id = %d\n", SDL_ThreadID());
        char *filename;
        filename = getTileFileName(tile -> provider, tile);
        if (!loadTileTexture(filename, tile)) {

            if (shouldAbortLoading(tile)) {
                free(filename);
                continue; 
            }

            //            fprintf(stderr, "load tile from local file failed, trying to download tile\n");
            ensureDirExists(filename);
            if (downloadAndSave(getTileUrl(tile -> provider, tile), filename)) {

                if (shouldAbortLoading(tile)) {
                    free(filename);
                    continue;
                }

                if (loadTileTexture(filename, tile)) {
                    tile ->transitionTime = options.downloadedFadeTime;
                    //                    fprintf(stderr, "download successfull\n");
                } else {
                    tile -> state = STATE_ERROR;
                    //                    fprintf(stderr, "unable to load downloaded tile\n");
                }
            } else {
                tile -> state = STATE_ERROR;
                //                fprintf(stderr, "download error\n");
            }
        } else {
            tile -> transitionTime = options.locallyLoadedFadeTime;
        }
        free(filename);
    }
    //    fprintf(stderr, "finished thread, id = %d\n", SDL_ThreadID());
    return 0;
}
