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

#ifdef N900
#include <SDL/SDL_gles.h>
#endif

#include <SDL/SDL.h>
#include "main.h"
#include "tile.h"
#include "tileengine.h"
#include "glunproject.h"
#include "texture.h"
#include "markerIterator.h"

#define MAX_HOLD_INDICATOR_POINTS 12
#define MAX_MARKERS 100
#define MAX_ROUTE_SEGMENTS 1000
#define QUADS_PER_ROUTE_SEGMENT 3
#define MAX_ROUTE_QUADS (QUADS_PER_ROUTE_SEGMENT*MAX_ROUTE_SEGMENTS)
#define MAX_QUADS MAX_ROUTE_QUADS
#define FLOATS_PER_VERTICE 3
#define VERTICES_PER_QUAD 4
#define FLOATS_PER_QUAD (FLOATS_PER_VERTICE*VERTICES_PER_QUAD)
#define VERTEX_INDICES_PER_QUAD 6
GLfloat mouseHoldIndicatorVertices[MAX_HOLD_INDICATOR_POINTS * FLOATS_PER_QUAD];

GLfloat searchMarkerVertices[MAX_MARKERS * FLOATS_PER_QUAD];
GLfloat shadowVertices[MAX_MARKERS * VERTICES_PER_QUAD * FLOATS_PER_VERTICE];
GLushort quadVertexIndices[MAX_QUADS * VERTEX_INDICES_PER_QUAD];
GLfloat quadTextureCoords[MAX_QUADS * VERTICES_PER_QUAD * 2];
ScreenMarker* renderedMarkers[MAX_MARKERS];

GLfloat routeVertices[MAX_ROUTE_QUADS * VERTICES_PER_QUAD * FLOATS_PER_VERTICE];
GLfloat routeTextCoords[MAX_ROUTE_QUADS * VERTICES_PER_QUAD * 2];

#define MAX_BUILDINGS_VERTICES (2000*18)
GLfloat buildingsVertices[MAX_BUILDINGS_VERTICES];
int wallCount;

Texture fontMediumMask, fontMedium;
Texture searchMarkTexture, searchMarkMask;
Texture routeStartTexture, routeStartMask;
Texture routeEndTexture, routeEndMask;
Texture shadow, dragCrosshair;
Texture road;

const int MEDIUM_FONT_METRICS[256] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 56, 0, 8, 8, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 5, 5, 11, 8, 12, 11, 3, 5, 5, 7, 11, 4, 5, 4, 4, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 4, 4, 11, 11, 11, 7, 13, 9, 9, 9, 10, 8, 7, 10, 10, 3, 3, 8, 7, 11, 10, 10, 8, 10, 8, 9, 7, 10, 9, 11, 10, 7, 10, 5, 4, 5, 11, 7, 7, 8, 8, 7, 8, 8, 4, 8, 8, 3, 3, 7, 3, 13,
		8, 8, 8, 8, 5, 7, 5, 8, 7, 9, 7, 7, 7, 8, 4, 8, 11, 8, 8, 8, 4, 8, 7, 13, 7, 7, 7, 17, 9, 5, 14, 8, 10, 8, 8, 4, 4, 7, 7, 8, 7, 13, 7, 13, 7, 5, 13, 8, 7, 7, 8, 5, 8, 8, 8, 8, 4, 7, 7, 13, 6,
		8, 11, 5, 13, 7, 7, 11, 5, 5, 7, 8, 8, 4, 7, 5, 6, 8, 13, 13, 13, 7, 9, 9, 9, 9, 9, 9, 13, 9, 8, 8, 8, 8, 3, 3, 3, 3, 10, 10, 10, 10, 10, 10, 10, 11, 10, 10, 10, 10, 10, 7, 8, 8, 8, 8, 8, 8,
		8, 8, 13, 7, 8, 8, 8, 8, 3, 3, 3, 3, 8, 8, 8, 8, 8, 8, 8, 11, 8, 8, 8, 8, 8, 7, 8, 7 };

GLfloat boxVertices[12] = { 0.0f, 0.0f, 0.0f, 0.0f, 256.0f, 0.0f, 256.0f, 256.0f, 0.0f, 256.0f, 0.0f, 0.0f, };

GLfloat fakeFogColorsLandscape[16] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 0.01f };
GLfloat fakeFogColorsPortrait[16] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.01f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.01f };

int debugTilesGridInitialized = FALSE;
GLfloat debugTilesGrid[(TILES_X + TILES_Y + 2 + MAX_ZOOM_HELPER_TILES * 2) * 3 * 2];

GLfloat zoomBarLines[40 * 3 * 2];

GLfloat quadTriangleVertices[18] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, };

GLfloat quadTriangleTexCoords[12] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, };

GLfloat quadStripVertices[12] = { 0.0f, 0.0f, 0.0f, /* Bottom Left Of The Quad */
1.0f, 0.0f, 0.0f, /* Bottom Right Of The Quad */
0.0f, 1.0f, 0.0f, /* Top Left Of The Quad */
1.0f, 1.0f, 0.0f, /* Top Right Of The Quad */
};

GLfloat arrowVertices[12] = { -8.0f, 24.0f, 0.0f, 0.0f, 8.0f, 0.0f, 0.0f, -24.0f, 0.0f, 8.0f, 24.0f, 0.0f, };
GLfloat directionVertices[9] = { 0.0f, 18.0f, 0.0f, -6.0f, 6.0f, 0.0f, 6.0f, 6.0f, 0.0f };

GLfloat texCoordsLandscape[][8] = { { 0.0f, 0.0f, /* Bottom Left Of The Texture */
1.0f, 0.0f, /* Bottom Right Of The Texture */
0.0f, 1.0f, /* Top Left Of The Texture */
1.0f, 1.0f, /* Top Right Of The Texture */
} };

GLfloat texCoordsPortrait[][8] = { { 1.0f, 0.0f, /* Top Right Of The Texture */
1.0f, 1.0f, /* Bottom Right Of The Texture */
0.0f, 0.0f, /* Bottom Left Of The Texture */
0.0f, 1.0f, /* Top Left Of The Texture */

} };

GLfloat texCoordsPartTile[8] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, };

SDL_Surface* loadTextureFromFile(char *filename, t_tile* tile, SDL_Surface** mask);

GLuint createTexture(GLushort * pixels4444, int w, int h, int isTile);

char strbuf[MAX_BUF_SIZE];
char strbuf2[MAX_BUF_SIZE];

void resetQuadStripVertices() {
	memset(quadStripVertices, 0, 12 * sizeof(GLfloat));
}

void setQuadStripSize(GLfloat w, GLfloat h, GLfloat quad[12]) {
	quad[3] = w;
	quad[7] = h;
	quad[9] = w;
	quad[10] = h;
}

void setBoxSize(GLfloat w, GLfloat h, GLfloat box[12]) {
	box[4] = h;
	box[6] = w;
	box[7] = h;
	box[9] = w;
}

int stringWidth(char * str) {
	int result = 0, i = 0;
	while (str[i] != '\0') {
		result += MEDIUM_FONT_METRICS[(int) str[i]];
		i++;
	}
	return result;
}

GLfloat stringVertices[MAX_BUF_SIZE * 18];
GLfloat stringTexCoords[MAX_BUF_SIZE * 12];

void drawStringAlpha(char* str, GLfloat r, GLfloat g, GLfloat b, GLfloat a, int useMask, GLenum sfactor, GLenum dfactor) {
	int i, len, charTileSize, xpos = 0;
	//    int j;
	len = strlen(str);

	int tx, ty;

	if (len == 0) {
		return;
	}

	charTileSize = fontMediumMask.size / 16;
	setQuadStripSize(charTileSize, charTileSize, quadStripVertices);

	for (i = 0; i < len; i++) {
		//        fprintf(stderr, "------------------------------\ni = %d\n", i);
		stringVertices[i * 18] = xpos;
		stringVertices[i * 18 + 1] = 0;
		stringVertices[i * 18 + 2] = 0;

		stringVertices[i * 18 + 3] = xpos;
		stringVertices[i * 18 + 4] = charTileSize;
		stringVertices[i * 18 + 5] = 0;

		stringVertices[i * 18 + 6] = xpos + charTileSize;
		stringVertices[i * 18 + 7] = charTileSize;
		stringVertices[i * 18 + 8] = 0;

		stringVertices[i * 18 + 9] = xpos;
		stringVertices[i * 18 + 10] = 0;
		stringVertices[i * 18 + 11] = 0;

		stringVertices[i * 18 + 12] = xpos + charTileSize;
		stringVertices[i * 18 + 13] = charTileSize;
		stringVertices[i * 18 + 14] = 0;

		stringVertices[i * 18 + 15] = xpos + charTileSize;
		stringVertices[i * 18 + 16] = 0;
		stringVertices[i * 18 + 17] = 0;

		xpos += MEDIUM_FONT_METRICS[(unsigned char) str[i]];
		//        fprintf(stderr, "Width of '%c' is %d. xpos = %d\n", str[i], MEDIUM_FONT_METRICS[(int)str[i]], xpos);

		//        for (j = 0; j < 18; j++) {
		//            fprintf(stderr, "stringVertices[%d] = %g\n", i * 18 + j, stringVertices[i * 18 + j]);
		//        }

		tx = (str[i] % 16) * charTileSize;
		ty = str[i] / 16 * charTileSize;

		stringTexCoords[i * 12 + 0] = tx / (GLfloat) fontMediumMask.size;
		stringTexCoords[i * 12 + 1] = ty / (GLfloat) fontMediumMask.size;

		stringTexCoords[i * 12 + 2] = tx / (GLfloat) fontMediumMask.size;
		stringTexCoords[i * 12 + 3] = (ty + charTileSize) / (GLfloat) fontMediumMask.size;

		stringTexCoords[i * 12 + 4] = (tx + charTileSize) / (GLfloat) fontMediumMask.size;
		stringTexCoords[i * 12 + 5] = (ty + charTileSize) / (GLfloat) fontMediumMask.size;

		stringTexCoords[i * 12 + 6] = tx / (GLfloat) fontMediumMask.size;
		stringTexCoords[i * 12 + 7] = ty / (GLfloat) fontMediumMask.size;

		stringTexCoords[i * 12 + 8] = (tx + charTileSize) / (GLfloat) fontMediumMask.size;
		stringTexCoords[i * 12 + 9] = (ty + charTileSize) / (GLfloat) fontMediumMask.size;

		stringTexCoords[i * 12 + 10] = (tx + charTileSize) / (GLfloat) fontMediumMask.size;
		stringTexCoords[i * 12 + 11] = ty / (GLfloat) fontMediumMask.size;

		//        for (j = 0; j < 12; j++) {
		//            fprintf(stderr, "textureCoords[%d] = %g\n", i * 12 + j, stringTexCoords[i * 12 + j]);
		//        }
	}
	glEnable(GL_BLEND);
	if (useMask) {
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		int glerror;
		glerror = glGetError();
		if (glerror != GL_NO_ERROR) {
			fprintf(stderr, "error 1 %d", glerror);
		}

		glColor4f(1.0, 1.0, 1.0, a);
		glBindTexture(GL_TEXTURE_2D, fontMediumMask.name);
		glVertexPointer(3, GL_FLOAT, 0, stringVertices);
		glTexCoordPointer(2, GL_FLOAT, 0, stringTexCoords);
		glDrawArrays(GL_TRIANGLES, 0, len * 6);
	}

	glColor4f(r, g, b, a);
	glBlendFunc(sfactor, dfactor);
	int glerror;
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 2 %d", glerror);
	}
	glBindTexture(GL_TEXTURE_2D, fontMedium.name);
	glVertexPointer(3, GL_FLOAT, 0, stringVertices);
	glTexCoordPointer(2, GL_FLOAT, 0, stringTexCoords);
	glDrawArrays(GL_TRIANGLES, 0, len * 6);
}

void inline drawString(char* str, GLfloat r, GLfloat g, GLfloat b, int useMask) {
	drawStringAlpha(str, r, g, b, 1.0, useMask, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

#include "uielement.c"

float color = 1.0;

void drawBoundingBox() {
	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0, 0, 1, 1.0f);

	GLfloat boundingTrapezoidVertices[12] = {
	// first point:
			(canvas.boundingTrapezoid[0].tilex - canvas.tilex) * TILE_SIZE + canvas.boundingTrapezoid[0].x, (canvas.boundingTrapezoid[0].tiley - canvas.tiley) * TILE_SIZE
					+ canvas.boundingTrapezoid[0].y, 0,
			// second point:
			(canvas.boundingTrapezoid[1].tilex - canvas.tilex) * TILE_SIZE + canvas.boundingTrapezoid[1].x, (canvas.boundingTrapezoid[1].tiley - canvas.tiley) * TILE_SIZE
					+ canvas.boundingTrapezoid[1].y, 0,
			// third point:
			(canvas.boundingTrapezoid[2].tilex - canvas.tilex) * TILE_SIZE + canvas.boundingTrapezoid[2].x, (canvas.boundingTrapezoid[2].tiley - canvas.tiley) * TILE_SIZE
					+ canvas.boundingTrapezoid[2].y, 0,
			// fourth point:
			(canvas.boundingTrapezoid[3].tilex - canvas.tilex) * TILE_SIZE + canvas.boundingTrapezoid[3].x, (canvas.boundingTrapezoid[3].tiley - canvas.tiley) * TILE_SIZE
					+ canvas.boundingTrapezoid[3].y, 0, };

	glVertexPointer(3, GL_FLOAT, 0, boundingTrapezoidVertices);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

}

int markersInitialized = FALSE;

void initQuadIndices(GLushort* indices, int count) {
	int i;
	for (i = 0; i < count; i++) {
		indices[VERTEX_INDICES_PER_QUAD * i + 0] = i * 4 + 0;
		indices[VERTEX_INDICES_PER_QUAD * i + 1] = i * 4 + 1;
		indices[VERTEX_INDICES_PER_QUAD * i + 2] = i * 4 + 2;
		indices[VERTEX_INDICES_PER_QUAD * i + 3] = i * 4 + 1;
		indices[VERTEX_INDICES_PER_QUAD * i + 4] = i * 4 + 2;
		indices[VERTEX_INDICES_PER_QUAD * i + 5] = i * 4 + 3;
	}
}

void initRoadTextureCoords() {
	int i, idx = 0;
	for (i = 0; i < MAX_ROUTE_SEGMENTS; i++) {
		// left quad
		routeTextCoords[idx++] = 0.0;
		routeTextCoords[idx++] = 0.0;

		routeTextCoords[idx++] = 0.0;
		routeTextCoords[idx++] = 1.0;

		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 0.0;

		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 1.0;

		// center quad
		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 0.0;

		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 1.0;

		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 0.0;

		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 1.0;

		// right quad
		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 0.0;

		routeTextCoords[idx++] = 0.5;
		routeTextCoords[idx++] = 1.0;

		routeTextCoords[idx++] = 1.0;
		routeTextCoords[idx++] = 0.0;

		routeTextCoords[idx++] = 1.0;
		routeTextCoords[idx++] = 1.0;

	}
}

void initQuadTextureCoords() {
	int i;
	for (i = 0; i < MAX_QUADS; i++) {
		memcpy(&quadTextureCoords[i * 8], texCoordsLandscape, 8 * sizeof(GLfloat));
	}
}

void inline setColor(GLfloat* array, int idx, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
	array[idx + 0] = r;
	array[idx + 1] = g;
	array[idx + 2] = b;
	array[idx + 3] = a;
}

double inline deg2rad(double degrees) {
	return degrees * M_PI / 180.0;
}

void rotate2d(GLfloat* x, GLfloat* y, double rotation) {
	GLfloat tx = *x * cos(deg2rad(rotation)) - *y * sin(deg2rad(rotation));
	GLfloat ty = *x * sin(deg2rad(rotation)) + *y * cos(deg2rad(rotation));
	*x = tx;
	*y = ty;
}

void rotateMarkerStripVertices(double angle, GLfloat rotx, GLfloat roty) {
	int i;

	for (i = 0; i < 4; i++) {
		GLfloat x2, y2;
		x2 = quadStripVertices[i * 3] - rotx;
		y2 = quadStripVertices[i * 3 + 1] - roty;
		rotate2d(&x2, &y2, angle);
		quadStripVertices[i * 3] = x2 + rotx;
		quadStripVertices[i * 3 + 1] = y2 + roty;
	}
}

int searchMarkersCount, routeMarkersCount;
int prepareMarkers() {
	int verticeIdx = 0, count = 0, i;

	setQuadStripSize(searchMarkTexture.size / canvas.scale, searchMarkTexture.size / canvas.scale, quadStripVertices);
	GLfloat markerRotation = getMarkerRotation();
	GLfloat size = searchMarkTexture.size / canvas.scale;
	GLfloat halfSize = size / 2.0;

	rotateMarkerStripVertices(markerRotation, halfSize, size);
	Point markerRaise;
	markerRaise.x = 0.0;
	markerRaise.y = -1.0;
	rotate2d(&markerRaise.x, &markerRaise.y, markerRotation);

	Point shadowRaise;
	shadowRaise.x = .5;
	shadowRaise.y = -.75;
	rotate2d(&shadowRaise.x, &shadowRaise.y, markerRotation);

	ScreenMarker* marker = resetMarkerIterator();
	searchMarkersCount = 0;
	routeMarkersCount = 0;

	while (marker != NULL) {
		if (marker -> visible) {
			renderedMarkers[count] = marker;
			GLfloat tx = marker -> screenCoordinate.x - halfSize + markerRaise.x * marker -> raiseLevel / canvas.scale;
			GLfloat ty = marker -> screenCoordinate.y - size + markerRaise.y * marker -> raiseLevel / canvas.scale;

			GLfloat tx2 = marker -> screenCoordinate.x - halfSize + shadowRaise.x * marker -> raiseLevel / canvas.scale;
			GLfloat ty2 = marker -> screenCoordinate.y - size + shadowRaise.y * marker -> raiseLevel / canvas.scale;

			for (i = 0; i < VERTICES_PER_QUAD; i++) {
				int shadowIdx = verticeIdx;
				searchMarkerVertices[verticeIdx++] = quadStripVertices[3 * i + 0] + tx;
				searchMarkerVertices[verticeIdx++] = quadStripVertices[3 * i + 1] + ty;
				searchMarkerVertices[verticeIdx++] = quadStripVertices[3 * i + 2];

				shadowVertices[shadowIdx++] = quadStripVertices[3 * i + 0] + tx2;
				shadowVertices[shadowIdx++] = quadStripVertices[3 * i + 1] + ty2;
				shadowVertices[shadowIdx++] = quadStripVertices[3 * i + 2];

			}

			if (marker -> type == SEARCH_RESULT) {
				searchMarkersCount++;
			} else {
				routeMarkersCount++;
			}

			count++;
			if (count >= MAX_MARKERS - 1) {
				break;
			}
		}
		marker = markerIteratorNext();
	}
	resetQuadStripVertices();
	return count;
}

void renderMarkerShadows(int count) {
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glTexCoordPointer(2, GL_FLOAT, 0, quadTextureCoords);
	glVertexPointer(3, GL_FLOAT, 0, shadowVertices);
	glColor4f(1, 1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, shadow.name);
	glDrawElements(GL_TRIANGLES, count * VERTEX_INDICES_PER_QUAD, GL_UNSIGNED_SHORT, quadVertexIndices);
}

void renderMarkers(int start, int count, GLuint mask, GLuint texture) {
glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glTexCoordPointer(2, GL_FLOAT, 0, &quadTextureCoords[VERTICES_PER_QUAD * start * 2]);
	glVertexPointer(3, GL_FLOAT, 0, &searchMarkerVertices[VERTEX_INDICES_PER_QUAD * start * 2]);
	glColor4f(1, 1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, mask);
	glDrawElements(GL_TRIANGLES, count * VERTEX_INDICES_PER_QUAD, GL_UNSIGNED_SHORT, quadVertexIndices);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawElements(GL_TRIANGLES, count * VERTEX_INDICES_PER_QUAD, GL_UNSIGNED_SHORT, quadVertexIndices);
}

void drawDragCrosshair() {
	GLfloat sizeX = dragCrosshair.size / canvas.scale;
	GLfloat sizeY = dragCrosshair.size / canvas.scale;
	setQuadStripSize(sizeX, sizeY, quadStripVertices);

	double rotation = getMarkerRotation();
	rotateMarkerStripVertices(rotation, sizeX / 2, sizeY / 2);

	int verticeIdx = 0, i;

	GLfloat tx = canvas.dragCrosshairMarker -> screenCoordinate.x - sizeX / 2.0;
	GLfloat ty = canvas.dragCrosshairMarker -> screenCoordinate.y - sizeY / 2.0;

	for (i = 0; i < VERTICES_PER_QUAD; i++) {
		searchMarkerVertices[verticeIdx++] = quadStripVertices[3 * i + 0] + tx;
		searchMarkerVertices[verticeIdx++] = quadStripVertices[3 * i + 1] + ty;
		searchMarkerVertices[verticeIdx++] = quadStripVertices[3 * i + 2];
	}
	resetQuadStripVertices();
	glBindTexture(GL_TEXTURE_2D, dragCrosshair.name);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glTexCoordPointer(2, GL_FLOAT, 0, quadTextureCoords);
	glVertexPointer(3, GL_FLOAT, 0, searchMarkerVertices);
	glDrawElements(GL_TRIANGLES, VERTEX_INDICES_PER_QUAD, GL_UNSIGNED_SHORT, quadVertexIndices);
}

void drawMouseHoldIndicator() {
	static int mouseHoldIndicatorVerticesInitialized = FALSE;

	if (mouseHoldIndicatorVerticesInitialized == FALSE) {
		int i, verticeIdx = 0;
		setQuadStripSize(12, 12, quadStripVertices);
		for (i = 0; i < MAX_HOLD_INDICATOR_POINTS; i++) {
			GLfloat tx = 0.0;
			GLfloat ty = -40;
			rotate2d(&tx, &ty, i * (360 / MAX_HOLD_INDICATOR_POINTS));
			int j;
			for (j = 0; j < VERTICES_PER_QUAD; j++) {
				mouseHoldIndicatorVertices[verticeIdx++] = quadStripVertices[3 * j + 0] + tx - position -> mask.size / 2;
				mouseHoldIndicatorVertices[verticeIdx++] = quadStripVertices[3 * j + 1] + ty - position -> mask.size / 2;
				mouseHoldIndicatorVertices[verticeIdx++] = quadStripVertices[3 * j + 2];
			}
		}
		mouseHoldIndicatorVerticesInitialized = TRUE;
	}
	if (canvas.mouseHoldInPlace > 0 && canvas.popup.popupEntries == NULL) {
		int numberOfPoints = (canvas.mouseHoldInPlace / (double) options.mouseHoldToPopupTime) * (MAX_HOLD_INDICATOR_POINTS + 2);

		if (numberOfPoints > MAX_HOLD_INDICATOR_POINTS) {
			numberOfPoints = MAX_HOLD_INDICATOR_POINTS;
		}

		if (numberOfPoints > 0) {
			glTranslatef(canvas.mouseHoldPositionSc.x, canvas.mouseHoldPositionSc.y, 0.0);
			glEnable(GL_TEXTURE_2D);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			glTexCoordPointer(2, GL_FLOAT, 0, quadTextureCoords);
			glVertexPointer(3, GL_FLOAT, 0, mouseHoldIndicatorVertices);
			glColor4f(1, 1, 1, 1);
			glBindTexture(GL_TEXTURE_2D, position -> mask.name);
			glScalef(1 / canvas.scale, 1 / canvas.scale, 1);
			glDrawElements(GL_TRIANGLES, numberOfPoints * VERTEX_INDICES_PER_QUAD, GL_UNSIGNED_SHORT, quadVertexIndices);
			glTranslatef(-canvas.mouseHoldPositionSc.x, -canvas.mouseHoldPositionSc.y, 0.0);
		}
	}
}

GLfloat getPopupEntryHeight() {
	return 2 * options.popupEntryMargin + fontMediumMask.size / 16;
}

void drawPopup() {
	if (canvas.popup.popupEntries == NULL) {
		return;
	}
	glLoadIdentity();
	glTranslatef(-SCREEN_WIDTH / 2 + (int) canvas.x - TILE_SIZE, -SCREEN_HEIGHT / 2 + (int) canvas.y - TILE_SIZE, -100);

	if (canvas.popup.changed) {
		canvas.popup.animationFrame = 0;
		GList* elem = canvas.popup.popupEntries;
		canvas.popup.height = 0;
		canvas.popup.width = 0;
		while (elem != NULL) {
			PopupMenuEntry* entry = elem -> data;

			canvas.popup.height += 2 * options.popupEntryMargin + fontMediumMask.size / 16;
			entry -> displayNameWidth = stringWidth(entry -> displayName);

			if (entry -> displayNameWidth > canvas.popup.width) {
				canvas.popup.width = entry -> displayNameWidth;
			}
			elem = elem -> next;
		}
		canvas.popup.width += 2 * options.popupEntryMargin;
		GLfloat x, y;
		x = canvas.mouseHoldPositionSc.x - canvas.popup.width / 2;
		y = canvas.mouseHoldPositionSc.y - canvas.popup.height / 2;
		rotate2d(&x, &y, -canvas.rotz);
		fprintf(stderr, "x = %f, y = %f\n", x, y);
		canvas.popup.x = x;
		canvas.popup.y = y;
		canvas.popup.changed = FALSE;
	}

	glDisable(GL_TEXTURE_2D);
	glTranslatef(canvas.popup.x, canvas.popup.y, 0);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	if (canvas.popup.animationFrame < 30) {
		canvas.popup.animationFrame++;
		GLfloat scale = 0.5 + 0.5 * sin(canvas.popup.animationFrame / 25.0 * M_PI / 2) * 1.10;
		glTranslatef(canvas.popup.width / 2.0 - canvas.popup.width / 2.0 * scale, canvas.popup.height / 2.0 - canvas.popup.height / 2.0 * scale, 0.0);
		//		fprintf(stderr, "scale = %f\n", scale);
		glScalef(scale, scale, scale);
		glColor4f(.4, .4, .4, .7 * scale * scale);
	} else {
		glColor4f(.4, .4, .4, .7);
	}

	setQuadStripSize(canvas.popup.width, canvas.popup.height, quadStripVertices);
	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_TEXTURE_2D);

	GLfloat popupEntryHeight = getPopupEntryHeight();

	GList* elem = canvas.popup.popupEntries;
	glTranslatef(canvas.popup.width / 2, options.popupEntryMargin, 0);
	while (elem != NULL) {
		PopupMenuEntry* entry = elem -> data;

		if (entry == canvas.popup.clickedPopupEntry) {
			glDisable(GL_TEXTURE_2D);
			glTranslatef(-canvas.popup.width / 2, -options.popupEntryMargin, 0);
			setQuadStripSize(canvas.popup.width, popupEntryHeight, quadStripVertices);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(.0, .0, 1, .5);
			glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glTranslatef(canvas.popup.width / 2, options.popupEntryMargin, 0);
			glEnable(GL_TEXTURE_2D);
		}

		glTranslatef(-entry -> displayNameWidth / 2, 0, 0);
		drawString(entry -> displayName, 1, 1, 1, FALSE);
		glTranslatef(entry -> displayNameWidth / 2, popupEntryHeight, 0);
		elem = elem -> next;
	}

	//	glEnableClientState(GL_COLOR_ARRAY);
	//	line(0,0,50,50,10,0.9,.0,.2,.7,.0,.0,TRUE);
	//	glDisableClientState(GL_COLOR_ARRAY);
}

void drawMarkers() {
	prepareMarkers();
	renderMarkerShadows(searchMarkersCount + routeMarkersCount);

	if (searchMarkersCount > 0) {
		renderMarkers(0, searchMarkersCount, searchMarkMask.name, searchMarkTexture.name);
	}

	int i = searchMarkersCount;
	while (i < searchMarkersCount + routeMarkersCount) {
		GLuint mask, texture;
		if (renderedMarkers[i] -> type == ROUTE_START) {
			mask = routeStartMask.name;
			texture = routeStartTexture.name;
		} else { //if (renderedMarkers[searchMarkersCount] -> type == ROUTE_START) {
			mask = routeEndMask.name;
			texture = routeEndTexture.name;
		}
		renderMarkers(i, 1, mask, texture);
		i++;
	}
	if (canvas.draggedMarker != NULL || canvas.dragCrosshairMarker != NULL) {
		if (canvas.draggedMarker != NULL) {
			canvas.dragCrosshairMarker = canvas.draggedMarker;
		}
		if (canvas.dragCrosshairMarker -> raiseLevel == 0) {
			canvas.dragCrosshairMarker = NULL;
		} else {
			drawDragCrosshair();
		}
	}
}

// http://answers.oreilly.com/topic/1669-how-to-render-anti-aliased-lines-with-textures-in-ios-4/
int prepareRouteTriangleVertices() {
	int i;
	GArray* array = canvas.route.optimizedRoute.lineStripTileCoordinates;
	if (array == NULL || array -> len < 2) {
		return 0;
	}
	int idx = 0;

	for (i = 0; i < array -> len - 1; i++) {
		if (i >= MAX_ROUTE_SEGMENTS) {
			return (i - 1);
		}
		TileCoordinate tc1 = g_array_index(array, TileCoordinate, i);
		TileCoordinate tc2 = g_array_index(array, TileCoordinate, i + 1);
		Point sc1;
		Point sc2;
		toScreenCoordinate(&tc1, &sc1);
		toScreenCoordinate(&tc2, &sc2);
		Point e; // east
		e.x = sc2.x - sc1.x;
		e.y = sc2.y - sc1.y;
		GLfloat dist = pointDistance(&sc1, &sc2);
		GLfloat scale;
		if (dist == 0.0) {
			e.x = 1.0;
			e.y = 0.0;
			scale = 1.0 / canvas.scale;
		} else {
			scale = 1.0 / dist / canvas.scale;
		}
		scale *= 4.0;

		if (canvas.zoom > 14) {
			scale *= 1.0 + (canvas.zoom - 16.0 + canvas.scale);
		}
		e.x *= scale;
		e.y *= scale;

		Point n; // north
		n.x = -e.y;
		n.y = e.x;

		Point s; // south
		s.x = -n.x;
		s.y = -n.y;

		Point w; // west
		w.x = -e.x;
		w.y = -e.y;

		Point ne; // north-east
		ne.x = n.x + e.x;
		ne.y = n.y + e.y;

		Point nw; // north-west
		nw.x = n.x + w.x;
		nw.y = n.y + w.y;

		Point se; // south-east
		se.x = s.x + e.x;
		se.y = s.y + e.y;

		Point sw; // south-west
		sw.x = s.x + w.x;
		sw.y = s.y + w.y;

		if (i == 0) {

		}

		// starting half-circle
		routeVertices[idx++] = sc1.x + sw.x;
		routeVertices[idx++] = sc1.y + sw.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc1.x + nw.x;
		routeVertices[idx++] = sc1.y + nw.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc1.x + s.x;
		routeVertices[idx++] = sc1.y + s.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc1.x + n.x;
		routeVertices[idx++] = sc1.y + n.y;
		routeVertices[idx++] = 0.0;

		// main road segment
		routeVertices[idx++] = sc1.x + s.x;
		routeVertices[idx++] = sc1.y + s.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc1.x + n.x;
		routeVertices[idx++] = sc1.y + n.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc2.x + s.x;
		routeVertices[idx++] = sc2.y + s.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc2.x + n.x;
		routeVertices[idx++] = sc2.y + n.y;
		routeVertices[idx++] = 0.0;

		// ending half-circle
		routeVertices[idx++] = sc2.x + s.x;
		routeVertices[idx++] = sc2.y + s.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc2.x + n.x;
		routeVertices[idx++] = sc2.y + n.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc2.x + se.x;
		routeVertices[idx++] = sc2.y + se.y;
		routeVertices[idx++] = 0.0;

		routeVertices[idx++] = sc2.x + ne.x;
		routeVertices[idx++] = sc2.y + ne.y;
		routeVertices[idx++] = 0.0;

	}

	return (array -> len - 1);
}

void renderRouteTriangles(int count) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glVertexPointer(3, GL_FLOAT, 0, routeVertices);
	glBindTexture(GL_TEXTURE_2D, road.name);
	glTexCoordPointer(2, GL_FLOAT, 0, routeTextCoords);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glDrawElements(GL_TRIANGLES, count * VERTEX_INDICES_PER_QUAD, GL_UNSIGNED_SHORT, quadVertexIndices);
}

void drawRoute() {
	int routeSegments = prepareRouteTriangleVertices();
	if (routeSegments > 0) {
		renderRouteTriangles(routeSegments * QUADS_PER_ROUTE_SEGMENT);
	}
}

void drawBuildings() {
	if (canvas.buildings == NULL) {
		return;
	}
	glPushMatrix();
	canvas.buildingsHeight = 20;
	int i, j, k, verticeCount;
	GLfloat translationX = (canvas.buildingsTileX - tiles[0][0][currentTilesIdx] -> tilex - getCanvasShiftX()) * TILE_SIZE;
	GLfloat translationY = (canvas.buildingsTileY - tiles[0][0][currentTilesIdx] -> tiley - getCanvasShiftY()) * TILE_SIZE;
	glTranslatef(translationX, translationY, 0.0);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4f(.4, .4, .4, .6);
	if (canvas.buildingsVerticesPrecomputed == 0) {
		wallCount = 0;
		for (i = 0; i < canvas.buildings -> len; i++) {
			GArray* polygon = g_array_index(canvas.buildings, GArray*, i);

			for (j = 0; j < polygon -> len - 1; j += 2) {

				//    1|\--------|3
				//     | \       |
				//     |  \      |  1st triangle = vertices 0, 1, 2
				//     |   \     |
				//     |    \    |  2nd triangle = vertices 3, 2, 1
				//     |     \   |
				//     |      \  |  vertices 0 and 1: x == polygon[j * 2], y == polygon[j * 2 + 1]
				//     |       \ |  vertices 2 and 3: x == polygon[k], y == polygin[k + 1], where k == j or k == 0 (joining last vertice with first one)
				//    0|--------\|2

				if (j + 2 < polygon -> len) {
					k = j + 2;
				} else {
					k = 0;
				}
				verticeCount = wallCount * 18;
				// 1st triangle, vertice 0
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, j );
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, j + 1);
				buildingsVertices[verticeCount++] = 0;

				// 1st triangle, vertice 1
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, j );
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, j + 1);
				buildingsVertices[verticeCount++] = canvas.buildingsHeight;

				// 1st triangle, vertice 2
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, k);
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, k + 1);
				buildingsVertices[verticeCount++] = 0;

				// 2nd triangle, vertice 2
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, k);
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, k + 1);
				buildingsVertices[verticeCount++] = 0;

				// 2nd triangle, vertice 1
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, j );
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, j + 1);
				buildingsVertices[verticeCount++] = canvas.buildingsHeight;

				// 2nd triangle, vertice 3
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, k);
				buildingsVertices[verticeCount++] = g_array_index(polygon, double, k + 1);
				buildingsVertices[verticeCount++] = canvas.buildingsHeight;

				//            fprintf(stderr, "building triangle %d (i=%d/%d, j=%d/%d, k=%d)\n", wallCount, i, canvas.buildings -> len, j, polygon -> len/2, k);
				//            for (k=0; k<18; k++) {
				//                fprintf(stderr, "%f, ", buildingsVertices[triangleCount * 18 + k]);
				//            }
				//
				//            fprintf(stderr, "\n");
				wallCount++;
				if (wallCount * 18 >= MAX_BUILDINGS_VERTICES) {
					break;
				}
			}
			if (wallCount * 18 >= MAX_BUILDINGS_VERTICES) {
				break;
			}
		}
		canvas.buildingsVerticesPrecomputed = 1;
	}
	glVertexPointer(3, GL_FLOAT, 0, buildingsVertices);
	glDrawArrays(GL_TRIANGLES, 0, wallCount * 6);
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void drawTile_debug(t_tile *tile) {
	if (tile == NULL) {
		return;
	}
	glDisable(GL_TEXTURE_2D);
	switch (tile -> state) {
	case STATE_GARBAGE:
		glColor4f(0, 0, 0, 1.0);
		break;
	case STATE_EMPTY:
		glColor4f(0, 0, 0, 0.5);
		break;
	case STATE_QUEUE_DELAYED:
		//		if (tile -> oldTiles[0] != NULL) {
		return;
		//glColor4f(0.5, 0, 0.5, 0.5);
		//		} else {
		//			glColor4f(.2, .0, .0, 0.5);
		//		}
		break;
	case STATE_QUEUED:
		glColor4f(.2, .2, .2, 0.5);
		break;
	case STATE_ERROR:
		glColor4f(1, 0, 0, 0.5);
		break;
	case STATE_LOADING:
		glColor4f(0, 0, 1, 0.5);
		break;
	case STATE_LOADED:
		glColor4f(0, 1, 0, 0.5);
		break;
	case STATE_GL_TEXTURE_NOT_CREATED:
		glColor4f(0, 0.2, 0, 0.5);
		break;
	case STATE_SCALED:
		glColor4f(0.5, 0.5, 0.5, 0.5);
		break;
	case STATE_LOADED_SYNC_WAIT:
		glColor4f(1, 0, 1, 0.5);
		break;

	}
	setQuadStripSize(TILE_SIZE, TILE_SIZE, quadStripVertices);
	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_TEXTURE_2D);

	//
	//	sprintf(strbuf, "%d", tile -> state);
	//	glScalef(16.0, 16.0, 16.0);
	//	drawString(strbuf, 0.1f, .0, 1.0, TRUE);
	//	glScalef(.0625, .0625, .0625);
}

void drawTile(t_tile* tile) {
	int i;
	if (tile == NULL) { //|| tile -> visible == FALSE || tile -> state == STATE_GARBAGE) {
		return;
	}
	if (tile -> texture == 0 && tile -> oldTiles[0] == NULL) {
		return;
	}

	if (tile->state == STATE_ERROR) {
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		setBoxSize(TILE_SIZE, TILE_SIZE, boxVertices);
		glVertexPointer(3, GL_FLOAT, 0, boxVertices);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		//        if (tile -> errorMessage) {
		//            sprintf(strbuf, "Error: %s", tile->errorMessage);
		//            drawString(strbuf, 1.0f, .2, .2, TRUE);
		//        }
	}

	// TODO tmp
	//	if ((tile -> state == STATE_LOADED) && nowMillis - tile->stateChangeTime < tile->transitionTime) {
	//		color = (nowMillis - tile->stateChangeTime) / (float) tile->transitionTime;
	//		color = 1.0 - sqrt(sqrt(1.0 - color));
	//	} else {
	color = 1.0;
	//	}

	// If there is old tile to display and loaded tile to blend with it use multitexturing for smoother transition.
	if (tile -> state == STATE_LOADED && color < 1.0 && tile -> oldTiles[0] != NULL) {
		volatile t_tile * oldTile = tile -> oldTiles[0];
		GLfloat *textcoords;

		setQuadStripSize(TILE_SIZE, TILE_SIZE, quadStripVertices);
		textcoords = texCoordsLandscape[0];

		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, oldTile -> texture);

		oldTile -> textureUsedTime = nowMillis;

		if (oldTile -> zoom < canvas.zoom) {
			int zoomDiff = canvas.zoom - oldTile -> zoom;
			GLfloat zoomDiffPow = pow(2.0, zoomDiff);
			GLfloat size = 1.0 / zoomDiffPow;
			GLfloat left = (tile -> tilex - (oldTile -> tilex * zoomDiffPow)) * size;
			GLfloat right = left + size;
			GLfloat up = (tile -> tiley - (oldTile -> tiley * zoomDiffPow)) * size;
			GLfloat down = up + size;
			texCoordsPartTile[0] = left;
			texCoordsPartTile[1] = up;
			texCoordsPartTile[2] = right;
			texCoordsPartTile[3] = up;
			texCoordsPartTile[4] = left;
			texCoordsPartTile[5] = down;
			texCoordsPartTile[6] = right;
			texCoordsPartTile[7] = down;
			glTexCoordPointer(2, GL_FLOAT, 0, texCoordsPartTile);
		} else {
			setQuadStripSize(TILE_SIZE, TILE_SIZE, quadStripVertices);
			glTexCoordPointer(2, GL_FLOAT, 0, texCoordsLandscape[0]);
		}

		glActiveTexture(GL_TEXTURE1);
		glClientActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tile -> texture);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
		glTexEnvf(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT);
		glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, textcoords);

		float mycolor[4];
		mycolor[0] = mycolor[1] = mycolor[2] = color;
		mycolor[3] = 1.0;
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, mycolor);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		//		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
		//		mycolor[0] = mycolor[1] = mycolor[2] = 0.0;
		//		mycolor[3] = 0.0;
		//		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, mycolor);
	} else {

		// Use blending if multitexturing is not applicable.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		int glerror;
		glerror = glGetError();
		if (glerror != GL_NO_ERROR) {
			fprintf(stderr, "error 3 %d", glerror);
		}
		if ((tile -> state != STATE_LOADED) || color < 1.0) {
			for (i = 0; i < 4; i++) {
				volatile t_tile * oldTile = tile -> oldTiles[i];
				if (oldTile != NULL) {
					oldTile -> textureUsedTime = nowMillis;
					if (oldTile -> zoom > canvas.zoom) {
						setQuadStripSize(TILE_SIZE / 2, TILE_SIZE / 2, quadStripVertices);
						glTexCoordPointer(2, GL_FLOAT, 0, texCoordsLandscape[0]);
						glTranslatef(TILE_SIZE / 2 * (i / 2), TILE_SIZE / 2 * (i % 2), 0);
					} else if (oldTile -> zoom < canvas.zoom) {
						setQuadStripSize(TILE_SIZE, TILE_SIZE, quadStripVertices);
						int zoomDiff = canvas.zoom - oldTile -> zoom;
						GLfloat zoomDiffPow = pow(2.0, zoomDiff);
						GLfloat size = 1.0 / zoomDiffPow;
						GLfloat left = (tile -> tilex - (oldTile -> tilex * zoomDiffPow)) * size;
						GLfloat right = left + size;
						GLfloat up = (tile -> tiley - (oldTile -> tiley * zoomDiffPow)) * size;
						GLfloat down = up + size;
						//                        fprintf(
						//                                stderr,
						//                                "oldtilex=%d, oldtiley=%d, tilex=%d, tiley=%d, oldzoom=%d, zoom=%d, zoomDiff=%d, zommDiffPow=%f, size=%f, left=%f, right=%f, up=%f, down=%f\n",
						//                                oldTile -> tilex, oldTile -> tiley, tile -> tilex, tile-> tiley, oldTile -> zoom, canvas.zoom, zoomDiff, zoomDiffPow,
						//                                size, left, right, up, down);
						texCoordsPartTile[0] = left;
						texCoordsPartTile[1] = up;
						texCoordsPartTile[2] = right;
						texCoordsPartTile[3] = up;
						texCoordsPartTile[4] = left;
						texCoordsPartTile[5] = down;
						texCoordsPartTile[6] = right;
						texCoordsPartTile[7] = down;
						glTexCoordPointer(2, GL_FLOAT, 0, texCoordsPartTile);
					} else {
						setQuadStripSize(TILE_SIZE, TILE_SIZE, quadStripVertices);
						glTexCoordPointer(2, GL_FLOAT, 0, texCoordsLandscape[0]);
					}

					if (tile -> state != STATE_LOADED) {
						glColor4f(1.0, 1.0, 1.0, 1.0);
					} else {
						glColor4f(1.0, 1.0, 1.0, 1.0 - color);
					}

					glBindTexture(GL_TEXTURE_2D, oldTile->texture);
					glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

					if (oldTile -> zoom > canvas.zoom) {
						glTranslatef(-TILE_SIZE / 2 * (i / 2), -TILE_SIZE / 2 * (i % 2), 0);
					}
				}
			}
			glColor4f(1.0, 1.0, 1.0, color);
		} else {
			glDisable(GL_BLEND);
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}

		if (tile -> state == STATE_LOADED) {
			if (tile -> texture == 0 && tile -> oldTiles[0] != NULL) {
				t_tile* oldtile = ((t_tile*) (tile->oldTiles[0]));
				glBindTexture(GL_TEXTURE_2D, oldtile->texture);
				oldtile -> textureUsedTime = nowMillis;
			} else if (tile -> texture != 0) {
				glBindTexture(GL_TEXTURE_2D, tile->texture);
				tile -> textureUsedTime = nowMillis;
			}
			setQuadStripSize(TILE_SIZE, TILE_SIZE, quadStripVertices);
			glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
			glTexCoordPointer(2, GL_FLOAT, 0, texCoordsLandscape[0]);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

	}

	if (options.showGrid) {
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		setBoxSize(TILE_SIZE, TILE_SIZE, boxVertices);
		glColor4f(0.7f, .7f, 0.7f, 1.0f);
		glVertexPointer(3, GL_FLOAT, 0, boxVertices);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
		glEnable(GL_TEXTURE_2D);
		int count = 0;
		for (i = 0; i < 4; i++) {
			if (tile -> oldTiles[i] != NULL) {
				if (((t_tile*) tile -> oldTiles[i]) -> texture != 0) {
					count++;
				}
			}
		}
		sprintf(strbuf, "x=%d, y=%d, state=%d, tc=%d", tile->tilex, tile -> tiley, tile -> state, count);
		drawString(strbuf, 0.1f, .0, 1.0, TRUE);
		glTranslatef(0, 16, 0);
		sprintf(strbuf, "tx=%d otx=%d vis=%d", tile->texture, (tile->oldTiles[0] != NULL ? ((t_tile*) (tile->oldTiles[0]))->texture : -1), tile->visible);
		drawString(strbuf, 0.1f, .0, 1.0, TRUE);
		glTranslatef(0, -16, 0);
	}
}

#ifdef N900
SDL_GLES_Context *context;
#endif

/* general OpenGL initialization function */
int initGL(GLvoid) {

	SDL_Surface *surface;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
		exit(1);
	}

//	if (TTF_Init() == -1) {
//		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
//		exit(2);
//	}


#if defined(N900)
	surface = SDL_SetVideoMode(800, 400, 16, SDL_SWSURFACE);
	SCREEN_WIDTH = surface->w;
	SCREEN_HEIGHT = surface->h;
#elif defined(N950)
	fprintf(stderr, "N950 opengl init\n");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	surface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_OPENGLES | SDL_FULLSCREEN);
#else
	surface = SDL_SetVideoMode(960, 460, 16, SDL_SWSURFACE);
#endif
	if (surface == NULL) {
		fprintf(stderr, "Couldn't set GL mode: %s\n", SDL_GetError());
		exit(-1);
	}
	SDL_ShowCursor(0);

	SDL_WM_SetCaption("CloudGPS", "cloudgps");
#if defined(N900)
	SDL_GLES_Init(SDL_GLES_VERSION_1_1);

	context = SDL_GLES_CreateContext();
	SDL_GLES_MakeCurrent(context);
#endif

	//	status = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
	//	if (status < 0) {
	//		fprintf(stderr, "Can't init default SDL video driver: %s\n", SDL_GetError());
	//		exit(-1);
	//	}

	/* Select first display */
	//    status=SDL_SelectVideoDisplay(0);
	//    if (status<0)
	//    {
	//        fprintf(stderr, "Can't attach to first display: %s\n", SDL_GetError());
	//        exit(-1);
	//    }
	//	window=SDL_CreateWindow("CloudGPS " CLOUDGPS_VERION,
	//			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	//			SCREEN_WIDTH, SCREEN_HEIGHT,
	//			SDL_WINDOW_OPENGL
	//			|SDL_WINDOW_SHOWN
	//#ifdef N900
	//			|SDL_WINDOW_FULLSCREEN
	//#endif
	//	);
	//	if (window == 0) {
	//		fprintf(stderr, "Can't create window: %s\n", SDL_GetError());
	//		exit(1);
	//	}
	//
	//	glcontext = SDL_GL_CreateContext(window);
	//	if (glcontext == NULL) {
	//		fprintf(stderr, "Can't create OpenGL (ES) context: %s\n", SDL_GetError());
	//		exit(1);
	//	}
	//
	//	status = SDL_GL_MakeCurrent(window, glcontext);
	//	if (status < 0) {
	//		fprintf(stderr, "Can't set current OpenGL ES context: %s\n", SDL_GetError());
	//		exit(1);
	//	}

	glEnable(GL_TEXTURE_2D);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_CLEAR);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glMatrixMode(GL_PROJECTION);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, (GLint) SCREEN_WIDTH, (GLint) SCREEN_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#if defined(N900) || defined(N950)
	glFrustumf(-(SCREEN_WIDTH/4), (SCREEN_WIDTH/4), (SCREEN_HEIGHT/4), -(SCREEN_HEIGHT/4), 50.0, 600);
#else
	//	GLfloat left, right, bottom, top, near=20, far=300;
	//	GLfloat fov = 135.0, aspect = SCREEN_WIDTH / (GLfloat) SCREEN_HEIGHT;
	//
	//	bottom = tan(fov*3.14159/360.0) * near;
	//	top = -bottom;
	//	right = aspect * bottom;
	//	left = aspect * top;

	//	glFrustum(left, right, bottom, top, near, far);
	glFrustum(-(SCREEN_WIDTH / 4), (SCREEN_WIDTH / 4), (SCREEN_HEIGHT / 4), -(SCREEN_HEIGHT / 4), 50.0, 600);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Real fog code - visually better but performance is much worse than fake fog
	//    GLfloat fogColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	//    glFogx(GL_FOG_MODE, GL_LINEAR); // Fog Mode
	//    glFogfv(GL_FOG_COLOR, fogColor); // Set Fog Color
	//    glFogf(GL_FOG_DENSITY, .1f); // How Dense Will The Fog Be
	//    glHint(GL_FOG_HINT, GL_FASTEST); // Fog Hint Value
	//    glFogf(GL_FOG_START, 100.0f); // Fog Start Depth
	//    glFogf(GL_FOG_END, 400.0f); // Fog End Depth
	//    glEnable(GL_FOG);
	//    const GLubyte *extensions = glGetString(GL_EXTENSIONS);
	//    fprintf(stderr, "GL extensions: %s\n", (const char*) extensions);

	initQuadIndices(quadVertexIndices, MAX_QUADS);
	initQuadTextureCoords();
	initRoadTextureCoords();
	return (TRUE);
}

GLfloat seconds;

void setupGlScene() {
	int pixelPerfect;

	clearR += (avgR - clearR) * 0.05;
	clearG += (avgR - clearG) * 0.05;
	clearB += (avgB - clearB) * 0.05;
	//    fprintf(stderr, "avgColor r=%f, g=%f, b=%f, clearColor r=%f, g=%f, b=%f\n", avgR, avgG, avgB, clearR, clearG, clearB);
	glClearColor(clearR, clearG, clearB, 1.0);

	glLoadIdentity();

	GLfloat scale = canvas.scale;
	//	if (fabs(canvas.rotx) > 0.1) {
	//		scale += fabs(canvas.rotx) / 35.0;
	//	}
	//	if (fabs(canvas.roty) > 0.1) {
	//		scale += fabs(canvas.roty) / 25.0;
	//	}
	glTranslatef(-(scale - 1.0) * SCREEN_WIDTH / 2 - SCREEN_WIDTH / 2, -(scale - 1.0) * SCREEN_HEIGHT / 2 - SCREEN_HEIGHT / 2, -100.0f);
	glScalef(scale, scale, scale);

	glTranslatef(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0);
	if (fabs(canvas.rotx) < 0.01 && (fabs(canvas.rotz) < 0.01 || fabs(canvas.rotz + 90.0) < 0.01) && fabs(canvas.roty) < 0.01) {
		glRotatef(canvas.rotz, 0.0, 0.0, 1.0);
		pixelPerfect = TRUE;
	} else {
		glRotatef(canvas.rotx, 1.0, 0.0, 0.0);
		glRotatef(canvas.roty, 0.0, 1.0, 0.0);
		glRotatef(canvas.rotz, 0.0, 0.0, 1.0);
		pixelPerfect = FALSE;
	}
	glTranslatef(-SCREEN_WIDTH / 2, -SCREEN_HEIGHT / 2, 0);

	if (canvas.scale - 1.0 < 0.01 && pixelPerfect) {
		// for pixel perfect representation of tiles in 2d mode
		glTranslatef((int) canvas.x - TILE_SIZE, (int) canvas.y - TILE_SIZE, 0.0f);
	} else {
		glTranslatef(canvas.x - TILE_SIZE, canvas.y - TILE_SIZE, 0.0f);
	}

	storeMatricesAndViewPort();
}

void drawDebugTiles() {
	int i, j;
	// Small grid of all tiles coloured by their state. Also a bounding trapezoid
	if (options.showDebugTiles) {
		glLoadIdentity();
		glTranslatef(300, -90, -100);
		glScalef(0.05, 0.05, 0.05);

		glEnable(GL_BLEND);
		//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		int glerror;
		glerror = glGetError();
		if (glerror != GL_NO_ERROR) {
			fprintf(stderr, "error 4 %d", glerror);
		}
		glTranslatef(-getCanvasShiftX() * TILE_SIZE, -getCanvasShiftY() * TILE_SIZE, 0);

		for (i = 0; i < TILES_X; i++) {
			for (j = 0; j < TILES_Y; j++) {
				glTranslatef(i * TILE_SIZE, j * TILE_SIZE, 0);
				drawTile_debug(tiles[i][j][currentTilesIdx]);
				glTranslatef(-i * TILE_SIZE, -j * TILE_SIZE, 0);
			}
		}

		for (i = 0; i < MAX_ZOOM_HELPER_TILES; i++) {
			glTranslatef(-3 * TILE_SIZE, i * TILE_SIZE, 0);
			drawTile_debug(zoomTiles[i][0]);
			glTranslatef(TILE_SIZE, 0, 0);
			drawTile_debug(zoomTiles[i][1]);
			glTranslatef(2 * TILE_SIZE, -i * TILE_SIZE, 0);
		}

		if (!debugTilesGridInitialized) {
			int idx = 0;
			// vertical lines for canvas grid
			for (i = 0; i < TILES_X + 1; i++) {
				debugTilesGrid[idx++] = i * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;
				debugTilesGrid[idx++] = 0.0;

				debugTilesGrid[idx++] = i * TILE_SIZE;
				debugTilesGrid[idx++] = TILES_Y * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;
			}

			// horizontal lines for canvas grid
			for (i = 0; i < TILES_Y + 1; i++) {
				debugTilesGrid[idx++] = 0.0;
				debugTilesGrid[idx++] = i * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;

				debugTilesGrid[idx++] = TILES_X * TILE_SIZE;
				debugTilesGrid[idx++] = i * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;
			}

			// vertical lines for zoom helper grid
			for (i = 0; i < 3; i++) {
				debugTilesGrid[idx++] = (i - 3) * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;
				debugTilesGrid[idx++] = 0.0;

				debugTilesGrid[idx++] = (i - 3) * TILE_SIZE;
				debugTilesGrid[idx++] = MAX_ZOOM_HELPER_TILES * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;
			}

			// horizontal lines for zoom helper grid
			for (i = 0; i < MAX_ZOOM_HELPER_TILES + 1; i++) {
				debugTilesGrid[idx++] = -3 * TILE_SIZE;
				debugTilesGrid[idx++] = i * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;

				debugTilesGrid[idx++] = -1 * TILE_SIZE;
				debugTilesGrid[idx++] = i * TILE_SIZE;
				debugTilesGrid[idx++] = 0.0;
			}

			debugTilesGridInitialized = TRUE;
		}
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.4, 0.4, 0.4, .7);
		glVertexPointer(3, GL_FLOAT, 0, debugTilesGrid);
		glDrawArrays(GL_LINES, 0, (TILES_X + TILES_Y + 2 + MAX_ZOOM_HELPER_TILES * 2) * 2);

		drawBoundingBox();
		glScalef(20, 20, 20);
		glTranslatef(-150, -20, 0);
		glEnable(GL_TEXTURE_2D);
		//		sprintf(strbuf, "routingStatus = %d", routingStatus);
		//		drawString(strbuf, .2, .0, 0.5, TRUE);
		glTranslatef(0, -20, 0);
		sprintf(strbuf, "minIdx: %d, flybyIdx: %d", canvas.navigationStatus.currentLineIdx, canvas.routeFlybyLineIdx);
		drawString(strbuf, .2, .0, 0.5, TRUE);
	}
}

void drawPosArrow() {
	GLfloat arrow[9];
	arrow[0] = canvas.arrowPosX;
	arrow[1] = canvas.arrowPosY;
	arrow[2] = 0.0;
	arrow[5] = 0.0;
	arrow[8] = 0.0;
	int size = 20;

	switch (canvas.arrowPos) {
	case 1: // left
		arrow[3] = canvas.arrowPosX + size;
		arrow[4] = canvas.arrowPosY - size / 2;

		arrow[6] = canvas.arrowPosX + size;
		arrow[7] = canvas.arrowPosY + size / 2;
		break;
	case 2: // right
		arrow[3] = canvas.arrowPosX - size;
		arrow[4] = canvas.arrowPosY - size / 2;

		arrow[6] = canvas.arrowPosX - size;
		arrow[7] = canvas.arrowPosY + size / 2;
		break;
	case 3: // up
		arrow[3] = canvas.arrowPosX - size / 2;
		arrow[4] = canvas.arrowPosY + size;

		arrow[6] = canvas.arrowPosX + size / 2;
		arrow[7] = canvas.arrowPosY + size;
		break;
	case 4: // down
		arrow[3] = canvas.arrowPosX - size / 2;
		arrow[4] = canvas.arrowPosY - size;

		arrow[6] = canvas.arrowPosX + size / 2;
		arrow[7] = canvas.arrowPosY - size;
		break;
	}

	glColor4f(.0, .0, 1.0, 0.6);
	glVertexPointer(3, GL_FLOAT, 0, &arrow);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void drawFog() {
	glLoadIdentity();
	glTranslatef(-SCREEN_WIDTH / 2, -SCREEN_HEIGHT / 2, -100);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	int glerror;
	glerror = glGetError();
	if (glerror != GL_NO_ERROR) {
		fprintf(stderr, "error 5 %d", glerror);
	}

	// landscape fake fog
	if (fabs(canvas.rotx) > 0.01) {
		glTranslatef(0, -((LANDSCAPE_ROTATION_X - canvas.rotx) / LANDSCAPE_ROTATION_X) * (SCREEN_HEIGHT / 2.5), 0);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, fakeFogColorsLandscape);
		setQuadStripSize(SCREEN_WIDTH, (SCREEN_HEIGHT / 2.5), quadStripVertices);
		glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableClientState(GL_COLOR_ARRAY);
		glTranslatef(0, +((LANDSCAPE_ROTATION_X - canvas.rotx) / LANDSCAPE_ROTATION_X) * (SCREEN_HEIGHT / 2.5), 0);
	}

	// portrait fake fog
	if (fabs(canvas.roty) > 0.01) {
		GLfloat fogStart = ((PORTRAIT_ROTATION_Y + canvas.roty) / PORTRAIT_ROTATION_Y) * (SCREEN_WIDTH / 2.5) - (SCREEN_WIDTH / 8);
		if (fogStart < 0.0) {
			glColor4f(0.0, 0.0, 0.0, 1.0);
			setQuadStripSize(-fogStart, SCREEN_HEIGHT, quadStripVertices);
			glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		glTranslatef(-fogStart, 0, 0);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 0, fakeFogColorsPortrait);
		setQuadStripSize((SCREEN_WIDTH / 2.5), SCREEN_HEIGHT, quadStripVertices);
		glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDisableClientState(GL_COLOR_ARRAY);
		glTranslatef(fogStart, 0, 0);
	}
}

void drawCoordinates() {
	glTranslatef(7, 7, 0);
	glColor4f(0.1, 0.1, 0.1, 0.6);
	sprintf(strbuf, "%04.6f %04.6f", canvas.center.latitude, canvas.center.longitude);

	setQuadStripSize(stringWidth(strbuf) + 10, 40, quadStripVertices);
	glVertexPointer(3, GL_FLOAT, 0, quadStripVertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glTranslatef(7, 5, 0);
	drawString(strbuf, 1.0, 1.0, 1.0, FALSE);
	glTranslatef(0, 15, 0);
	double distance = getDistance(&canvas.center, &canvas.currentPos);

	if (distance < 1000) {
		sprintf(strbuf, "distance = %.0fm", distance);
	} else if (distance < 100000) {
		sprintf(strbuf, "distance = %.2fkm", distance / 1000);
	} else {
		sprintf(strbuf, "distance = %.0fkm", distance / 1000);
	}
	drawString(strbuf, 1.0, 1.0, 1.0, FALSE);
}

void drawMovementDirectionArrow() {
	// Arrow showing direction of movement
#ifdef N900
	if ((device -> fix -> fields | LOCATION_GPS_DEVICE_TRACK_SET) && device -> satellites_in_use > 0 && device -> fix -> speed > 1) {
		GLfloat track = device -> fix -> track - 180.0;
		glTranslatef(position -> x + position -> texture.size / 2 / canvas.scale, position -> y + position -> texture.size / 2 / canvas.scale,
				0.0);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.09, 0.59, .94, 1.0);
		glRotatef(track, 0.0, 0.0, 1.0);
		directionVertices[1] = 18.0f / canvas.scale;
		directionVertices[3] = -6.0f / canvas.scale;
		directionVertices[4] = 6.0f / canvas.scale;
		directionVertices[6] = 6.0f / canvas.scale;
		directionVertices[7] = 6.0f / canvas.scale;

		glVertexPointer(3, GL_FLOAT, 0, directionVertices);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glRotatef(-track, 0.0, 0.0, 1.0);
		glTranslatef(-position -> x - position -> texture.size / 2, -position -> y - position -> texture.size / 2, 0.0);
	}
#endif
}

void computeFps() {
	static GLint oldTicks = 0;
	static GLint frames = 0;
	frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - oldTicks >= 1000) {
			seconds = (t - oldTicks) / 1000.0;
			fps = frames / seconds;
			oldTicks = t;
			frames = 0;
			//            fprintf(stderr, "fps: %f\n", fps);
		}
	}
}

void swapBuffers() {
#ifdef N900
	SDL_GLES_SwapBuffers();
#else
	SDL_GL_SwapBuffers();
#endif
}

void drawTiles() {
	int i, j;
	for (i = 0; i < TILES_X; i++) {
		for (j = 0; j < TILES_Y; j++) {
			if (tiles[i][j][currentTilesIdx] != NULL) {
				if (tiles[i][j][currentTilesIdx] -> visible == TRUE) {
					glTranslatef((i - getCanvasShiftX()) * TILE_SIZE, (j - getCanvasShiftY()) * TILE_SIZE, 0);
					drawTile(tiles[i][j][currentTilesIdx]);
					glTranslatef(-(i - getCanvasShiftX()) * TILE_SIZE, -(j - getCanvasShiftY()) * TILE_SIZE, 0);
				}
			}
		}
	}

	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void drawGLScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		fprintf(stderr, "drawGLScene GL_ERROR: %x\n", err);
	}

	drawTiles();
	drawBuildings();

//printf("position status: %u\n", position -> status);
	if (position -> status != UI_HIDDEN) {
		// Current position
		drawUiElement(position); //that brakes marker and route rendering
		drawMovementDirectionArrow();
	}

	drawRoute();
	drawMarkers();
	drawMouseHoldIndicator();
	drawPopup();
	drawDebugTiles();
	drawFog();
	drawUiElems();
	drawZoomBar();
	drawSearchBar();
	//glLoadIdentity();
	drawStatusBar(LANDSCAPE);
	glLoadIdentity();
	glTranslatef(-SCREEN_WIDTH / 2, -SCREEN_HEIGHT / 2, -100);
	drawStatusBar(PORTRAIT);

	glLoadIdentity();
	glTranslatef(0, 0, -100);
	if (canvas.arrowPos) {
		drawPosArrow();
	}
	if (options.showCoordinates) {
		drawCoordinates();
	}
	computeFps();
	swapBuffers();
}

