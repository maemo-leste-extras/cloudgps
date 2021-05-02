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

#include "network.h"
#include <zlib.h>

#define CLOUDMADE_BUILDINGS_SVG_QUERY_URL_XXXXXX "http://alpha.vectors.cloudmade.com/8ee2a50541944fb9bcedded5165f09d9/19.921,50.088,19.933,50.082/polygon/%28%22building%22%20IS%20NOT%20NULL%29?zoom=18"
#define CLOUDMADE_BUILDINGS_SVG_QUERY_URL "http://alpha.vectors.cloudmade.com/8ee2a50541944fb9bcedded5165f09d9/%f,%f,%f,%f/polygon/%%28%%22building%%22%%20IS%%20NOT%%20NULL%%29"
#define CLOUDMADE_BUILDINGS_TMP_FILE "/tmp/cloudgps-buildings.svg"
#define TMP_FILE_UNCOMPRESSED_SIZE (2*1024*1024)

typedef struct {
	double scale, translate[2], width, height;
	GArray *polygons; // data == GArray with double x and y values
} t_buildings3d;

void buildings_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error) {
	t_buildings3d *result = user_data;
	const gchar **name_cursor = attribute_names;
	const gchar **value_cursor = attribute_values;
	gchar *tmp;

	while (*name_cursor) {
		//        fprintf(stderr, "element '%s', attribute '%s', value '%s'\n", element_name, *name_cursor, *value_cursor);
		if (strcmp(element_name, "svg") == 0) {
			if (strcmp(*name_cursor, "width") == 0) {
				result -> width = strtod(*value_cursor, NULL);
				//                fprintf(stderr, "width = %f\n", result -> width);
			} else if (strcmp(*name_cursor, "height") == 0) {
				result -> height = strtod(*value_cursor, NULL);
				//                fprintf(stderr, "height = %f\n", result -> height);
			}
		}
		if (strcmp(element_name, "g") == 0) {
			if (strcmp(*name_cursor, "transform") == 0) {
				if (strncmp(*value_cursor, "scale(", 6) == 0) {
					result -> scale = strtod(*value_cursor + 6, NULL);
					//                    fprintf(stderr, "scale = %f\n", result -> scale);
				} else if (strncmp(*value_cursor, "translate(", 10) == 0) {
					result -> translate[0] = strtod(*value_cursor + 10, &tmp);
					//                    fprintf(stderr, "translateX = %f\n", result -> translate[0]);
					result -> translate[1] = strtod(&tmp[1], NULL);
					//                    fprintf(stderr, "translateY = %f\n", result -> translate[1]);
				}
			}
		}
		if (strcmp(element_name, "path") == 0) {
			if (strcmp(*name_cursor, "d") == 0) {
				tmp = (gchar*) *value_cursor;
				GArray *vertices = g_array_new(FALSE, FALSE, sizeof(double));
				while (tmp != NULL) {
					while (tmp[0] == ' ' || tmp[0] == 'L' || tmp[0] == 'M') {
						tmp++;
					}
					if (tmp[0] == 'Z' || tmp[0] == '\0') {
						g_array_append_val(result -> polygons, vertices);
						break;
					}
					double vertice;
					vertice = strtod(tmp, &tmp);
					vertice += result -> translate[vertices -> len % 2];
					vertice *= result -> scale;
					vertice *= (TILES_X * TILE_SIZE) / result -> width;

					//                    fprintf(stderr, "vertice %f\n", vertice);
					g_array_append_val(vertices, vertice);
				}
			}

		}
		name_cursor++;
		value_cursor++;
	}
}

void buildings_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error) {
}

void buildings_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error) {
}

void buildings_parse_error(GMarkupParseContext *context, GError *error, gpointer user_data) {
	char buf[300];
	sprintf(buf, "XML parse error: %s\n", error -> message);
	fprintf(stderr, "%s", buf);
	addConsoleLine(buf, 1, 0, 0);
}

/* The list of what handler does what. */
static GMarkupParser parser = { buildings_start_element, buildings_end_element, buildings_text, NULL, &buildings_parse_error };

void clearBuildings() {
	int i;
	if (canvas.buildings != NULL) {
		for (i = 0; i < canvas.buildings -> len; i++) {
			g_array_free (g_array_index(canvas.buildings, GArray*, i), TRUE);
		}
		g_array_free(canvas.buildings, TRUE);
		canvas.buildings = NULL;
	}
}

void buildings3dtest() {
	char *url, *svg;
	gsize size;
	clearBuildings();

	WorldCoordinate upperLeftWorld, lowerRightWorld;
	TileCoordinate upperLeftTile, lowerRightTile;
	upperLeftTile.tilex = tiles[0][0][currentTilesIdx] -> tilex;
	upperLeftTile.tiley = tiles[0][0][currentTilesIdx] -> tiley;
	upperLeftTile.zoom = canvas.zoom;
	upperLeftTile.x = 0.0;
	upperLeftTile.y = 0.0;
	lowerRightTile.tilex = tiles[TILES_X - 1][TILES_Y - 1][currentTilesIdx] -> tilex + 1;
	lowerRightTile.tiley = tiles[TILES_X - 1][TILES_Y - 1][currentTilesIdx] -> tiley + 1;
	lowerRightTile.zoom = canvas.zoom;
	lowerRightTile.x = 0.0;
	lowerRightTile.y = 0.0;
	toWorldCoordinate(&upperLeftTile, &upperLeftWorld);
	toWorldCoordinate(&lowerRightTile, &lowerRightWorld);

	if (asprintf(&url, CLOUDMADE_BUILDINGS_SVG_QUERY_URL, upperLeftWorld.longitude, upperLeftWorld.latitude, lowerRightWorld.longitude,
					lowerRightWorld.latitude) < 0) {
		fprintf(stderr, "asprintf failed in buildings3dtest\n");
		return;
	}
	if (downloadAndSave(url, CLOUDMADE_BUILDINGS_TMP_FILE)) {
		gzFile file = gzopen(CLOUDMADE_BUILDINGS_TMP_FILE, "rb");
		svg = malloc(TMP_FILE_UNCOMPRESSED_SIZE);
		if (svg == NULL) {
			fprintf(stderr, "Memory allocation error in buildings3dtest\n");
			return;
		}
		size = gzread(file, svg, TMP_FILE_UNCOMPRESSED_SIZE);
		//fprintf(stderr, "uncompressed: %*s\nsize: %d", size, svg, size);
		t_buildings3d *result = calloc(1, sizeof(t_buildings3d));
		result -> polygons = g_array_new(FALSE, FALSE, sizeof(GArray*));
		GMarkupParseContext *context = g_markup_parse_context_new(&parser, 0, result, NULL);
		//fprintf(stderr, "Context created\n");
		if (g_markup_parse_context_parse(context, svg, size, NULL) == FALSE) {
			g_markup_parse_context_free(context);
			free(svg);
			return;
		}
		canvas.buildingsTileX = upperLeftTile.tilex;
		canvas.buildingsTileY = upperLeftTile.tiley;
		canvas.buildingsVerticesPrecomputed = 0;
		canvas.buildings = result -> polygons;
		fprintf(stderr, "Parse success\n");
		g_markup_parse_context_free(context);
		free(svg);
	}
	remove(CLOUDMADE_BUILDINGS_TMP_FILE);
}
