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
#include <SDL/SDL_image.h>

#include "main.h"
#include "tile.h"

double avgR = 1, avgG = 1, avgB = 1, clearR = 1, clearG = 1, clearB = 1;

/* function to load in bitmap as SDL surface ready to be used in OpenGL */
SDL_Surface* loadTextureFromFile(char *filename, t_tile* tile, SDL_Surface** mask) {
	SDL_Surface *surface;
	SDL_Surface *surfaceRGB;
	SDL_Rect rect;


	if ((surface = IMG_Load(filename))) {

		// convert loaded image to RGBA image and then create opengl texture

		if (mask != NULL) {
			(*mask) = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, 16, 0x00000F000, 0x00000F00, 0x000000F0, 0x000000F);
			if ((*mask) == NULL) {
				fprintf(stderr, "Unable to create mask surface.\n");
				exit(-1);
			}
			int i;
			GLubyte * inputPixels = surface -> pixels;
			GLushort * outputPixels = (*mask) -> pixels;
			GLushort alpha;

			for (i = 0; i < surface -> w * surface -> h; i++) {
				alpha = 15 - (inputPixels[i * 4 + 3] >> 4);
				outputPixels[i] = alpha << 12 | alpha << 8 | alpha << 4 | 0xF;
			}
		}

		surfaceRGB = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, 16, 0x00000F000, 0x00000F00, 0x000000F0, 0x000000F);

		if (surfaceRGB) {
			rect.x = 0;
			rect.y = 0;
			rect.w = surface->w;
			rect.h = surface->h;
			SDL_UpperBlit(surface, &rect, surfaceRGB, &rect);

			if (tile != NULL && tile -> visible) {
				// Take one sample pixel from texture to estimate average color.
				// TODO Take few pixels
				int x = TILE_SIZE / 2, y = TILE_SIZE / 2;
				double r = 0, g = 0, b = 0, divisor;
				GLushort * pixels4444 = surfaceRGB -> pixels;
				r += ((pixels4444[y * TILE_SIZE + x] & 0xF000) >> 12) / 16.0;
				g += ((pixels4444[y * TILE_SIZE + x] & 0xF00) >> 8) / 16.0;
				b += ((pixels4444[y * TILE_SIZE + x] & 0xF0) >> 4) / 16.0;

				if (canvas.zoom < 1) {
					divisor = 1;
				} else if (canvas.zoom < 2) {
					divisor = 4;
				} else {
					divisor = 8;
				}
				avgR += (1.0 / divisor) * (r - avgR);
				avgG += (1.0 / divisor) * (g - avgG);
				avgB += (1.0 / divisor) * (b - avgB);
				//                fprintf(stderr, "r=%.4f, g=%.4f, b=%.4f\n", avgR, avgG, avgB);
			}

			SDL_FreeSurface(surface);

			return surfaceRGB;
		} else {
			fprintf(stderr, "createRGBsurface failed \n");
		}
	} else {
		if (tile == NULL) {
			// TODO if file exists error message should be written
			char* errorMessage;
			errorMessage = (char*) SDL_GetError();
			fprintf(stderr, "loading from file '%s' failed, error: %s\n", filename, errorMessage);
			//			remove(filename);
		}
	}
	//free(filename);
	return NULL;
}

GHashTable* textureSizeHashTable = NULL;

volatile long allocatedTexturesSize = 0;

void deleteTexture(GLuint texture) {
	int textureSize = (int) g_hash_table_lookup(textureSizeHashTable, (gpointer)texture);
	//	fprintf(stderr, "delete texture %d, size = %d, allocated total = %ld\n", texture, textureSize, allocatedTexturesSize);
	glDeleteTextures(1, &texture);
	allocatedTexturesSize -= textureSize;
}

GLuint createTexture(const GLushort * pixels4444, int w, int h, int isTile) {
	GLuint texture;

	if (textureSizeHashTable == NULL) {
		textureSizeHashTable = g_hash_table_new(NULL, NULL);
		if (textureSizeHashTable == NULL) {
			fprintf(stderr, "texture size hash table creation error\n");
			exit(1);
		}
	}

	glGenTextures(1, &texture);

	if (isTile) {
		g_hash_table_insert(textureSizeHashTable, (gpointer)texture, (gpointer)(w * h * 2));
		allocatedTexturesSize += w * h * 2;
		//		fprintf(stderr, "create texture %d, size = %d, allocated total = %ld\n", texture, w*h*2, allocatedTexturesSize);
	}

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	if (isTile && options.mipmapToggle) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, pixels4444);

	return texture;
}

long lastTileSync = 0;

// When changing zoom level a lot of tiles start loading at the same time. They usually
// all load within 1 second but we want to synchronize their state change so that they will
// start showing on screen simultaneuously.
void updateSyncWaitTiles() {
	if (syncLoadedTilesAppearance == FALSE) {
		return;
	}
	int i, j;
	int loadingCount = 0, syncWaitCount = 0;
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			if (tiles[i][j][currentTilesIdx] != NULL) {
				if (tiles[i][j][currentTilesIdx] -> state == STATE_LOADING && tiles[i][j][currentTilesIdx] -> visible) {
					loadingCount++;
				} else if (tiles[i][j][currentTilesIdx] -> state == STATE_LOADED_SYNC_WAIT) {
					syncWaitCount++;
				}
			}
		}
	}

	if (lastTileSync == 0 && (loadingCount > 0 || syncWaitCount > 0)) {
		lastTileSync = nowMillis;
	}

	if ((lastTileSync != 0 && nowMillis - lastTileSync > 1000 && syncWaitCount > 0)) {
		for (i = 0; i < TILES_X; i++) {
			for (j = 0; j < TILES_Y; j++) {
				if (tiles[i][j][currentTilesIdx] != NULL) {
					if (tiles[i][j][currentTilesIdx] -> state == STATE_LOADED_SYNC_WAIT) {
						tiles[i][j][currentTilesIdx]->state = STATE_LOADED;
						tiles[i][j][currentTilesIdx]->stateChangeTime = nowMillis;
						//fprintf(stderr, "%ld - synced tile %d, %d\n", nowMillis, i, j);
					}
				}
			}
		}
		if (loadingCount == 0) {
			lastTileSync = 0;
			syncLoadedTilesAppearance = FALSE;
		} else {
			lastTileSync = nowMillis;
		}
	}
}
