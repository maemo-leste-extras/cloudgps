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

GList* tileProviders = NULL;

void toQuadKey(TileCoordinate* c, char firstDigit, char* out);

char* getTileUrl(TileProvider *provider, t_tile *tile) {
    char* url = calloc(300, sizeof(char));
    if (strcmp(provider -> urlFormatType, "QUAD_KEY") == 0) {
		char* quadKey = calloc(32, sizeof(char));
    	TileCoordinate tc;
    	tc.tilex = tile -> tilex;
    	tc.tiley = tile -> tiley;
    	tc.zoom = tile -> zoom;
    	toQuadKey(&tc, provider -> quadKeyFirstChar, quadKey);
		sprintf(url, provider -> urlFormat, quadKey);
		free(quadKey);
    } else if (strcmp(provider -> urlFormatType, "OSM") == 0) {
		sprintf(url, provider -> urlFormat, tile->zoom, tile->tilex, tile->tiley);
    } else {
    	fprintf(stderr, "Invalid urlFormatType: '%s' for tile provider: '%s'\n", provider -> urlFormatType, provider -> name);
    	exit(1);
    }

    return url;
}

char* getTileFileName(TileProvider *provider, t_tile *tile) {
    char* filename = calloc(200, sizeof(char));
    sprintf(filename, provider -> filenameFormat, tile->zoom, tile->tilex, tile->tiley);
    return filename;
}
